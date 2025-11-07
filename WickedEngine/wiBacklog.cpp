#include "wiBacklog.h"
#include "wiMath.h"
#include "wiResourceManager.h"
#include "wiTextureHelper.h"
#include "wiFont.h"
#include "wiSpriteFont.h"
#include "wiImage.h"
#include "wiLua.h"
#include "wiInput.h"
#include "wiPlatform.h"
#include "wiHelper.h"
#include "wiGUI.h"
#include "wiTimer.h"

#include <mutex>
#include <deque>
#include <limits>
#include <iostream>
#include <thread>
#include <condition_variable>
#include <atomic>
#include <fstream>
#include <chrono>

using namespace wi::graphics;

namespace wi::backlog
{
	bool enabled = false;
	bool was_ever_enabled = enabled;
	const float speed = 4000.0f;
	const size_t deleteFromLine = 500; // Only limits in-memory display buffer, not file
	float pos = 5;
	float scroll = 0;
	int historyPos = 0;
	wi::font::Params font_params;
	wi::Color backgroundColor = wi::Color(17, 30, 43, 255);
	Texture backgroundTex;
	std::atomic<bool> refitscroll = false;
	wi::gui::TextInputField inputField;
	wi::gui::Button toggleButton;
	wi::gui::GUI GUI;

	bool locked = false;
	bool blockLuaExec = false;
	LogLevel logLevel = LogLevel::Default;
	std::atomic<LogLevel> unseen = LogLevel::None;

	std::deque<LogEntry> history;
	std::mutex historyLock;
	std::string logfile_path = "";

	struct AsyncWriter
	{
		std::thread writerThread;
		std::mutex queueMutex;
		std::condition_variable queueEntriesCondition;
		std::condition_variable queueEmptyCondition;
		std::deque<std::string> writeQueue;
		std::atomic<bool> running{ false };
		std::atomic<bool> initialized{ false };
		std::atomic<uint32_t> autoFlushInterval{ 1000 };
		wi::Timer lastFlushTimer;
		std::ofstream logFileStream;
		std::string currentLogFilePath;
		std::mutex flushMutex; // Synchronous flush requests

		void Start(const std::string& filepath)
		{
			if (initialized.exchange(true))
			{
				return;
			}

			currentLogFilePath = filepath;
			running = true;
			lastFlushTimer.record();

			logFileStream.open(filepath, std::ios::binary | std::ios::trunc);
			if (!logFileStream.is_open())
			{
				wi::helper::DebugOut("Failed to open log file: " + filepath + "\n", wi::helper::DebugLevel::Error);
			}

			writerThread = std::thread([this]() {
				WriterLoop();
			});
		}

		void Stop()
		{
			if (!initialized.load())
			{
				return;
			}

			running = false;
			queueEntriesCondition.notify_all();

			if (writerThread.joinable())
			{
				writerThread.join();
			}

			// Final flush of any remaining entries
			FlushQueue();

			if (logFileStream.is_open())
			{
				logFileStream.flush();
				logFileStream.close();
			}

			initialized = false;
		}

		void WriterLoop()
		{
			while (running.load())
			{
				std::unique_lock lock(queueMutex);

				// Wait for new entries or timeout for periodic flush
				auto timeout = std::chrono::milliseconds(100);
				queueEntriesCondition.wait_for(lock, timeout, [this]() {
					return !writeQueue.empty() || !running.load();
				});

				if (!running.load() && writeQueue.empty())
					break;

				// Process all queued entries
				static std::deque<std::string> localQueue;
				std::swap(localQueue, writeQueue);
				lock.unlock();

				// Notify waiters that queue is now empty after swap
				queueEmptyCondition.notify_all();

				WriteEntries(localQueue);
				localQueue.clear();

				// Check for auto-flush interval
				const auto interval = autoFlushInterval.load();
				if (interval > 0 && lastFlushTimer.elapsed_milliseconds() >= interval)
				{
					if (logFileStream.is_open())
					{
						logFileStream.flush();
					}
					lastFlushTimer.record();
				}
			}
		}

		void WriteEntries(const std::deque<std::string>& entries)
		{
			if (!logFileStream.is_open())
			{
				return;
			}

			for (const auto& entry : entries)
			{
				logFileStream.write(entry.c_str(), entry.length());
			}
		}

		void FlushQueue()
		{
			std::unique_lock lock(queueMutex);
			static thread_local std::deque<std::string> localQueue;
			std::swap(localQueue, writeQueue);
			lock.unlock();

			WriteEntries(localQueue);
			localQueue.clear();

			if (logFileStream.is_open())
			{
				logFileStream.flush();
			}
			lastFlushTimer.record();
		}

		void Enqueue(const std::string& text)
		{
			{
				std::scoped_lock lock(queueMutex);
				writeQueue.push_back(text);
			}
			queueEntriesCondition.notify_one();
		}

		void FlushSync()
		{
			std::scoped_lock lock(flushMutex);
			if (!initialized.load())
			{
				return;
			}

			// Signal and wait for queue to be processed
			std::unique_lock queueLock(queueMutex);
			if (!writeQueue.empty())
			{
				queueEntriesCondition.notify_one();
				queueEmptyCondition.wait(queueLock, [this] { return writeQueue.empty(); });
			}
			queueLock.unlock();

			// Force file flush
			if (logFileStream.is_open())
			{
				logFileStream.flush();
			}
		}

		void SetAutoFlushInterval(uint32_t ms)
		{
			autoFlushInterval = ms;
		}

		void UpdateLogFilePath(const std::string& newPath)
		{
			std::scoped_lock lock(flushMutex);

			// Flush and close current file
			FlushQueue();
			if (logFileStream.is_open())
			{
				logFileStream.close();
			}

			// Open new file
			currentLogFilePath = newPath;
			logFileStream.open(newPath, std::ios::binary | std::ios::trunc);
		}
	};

	static AsyncWriter asyncWriter;

	struct InternalState
	{
		// These must have common lifetime and destruction order, so keep them together in a struct:
		std::deque<LogEntry> entries;
		std::mutex entriesLock;

		InternalState() = default;
		~InternalState()
		{
			asyncWriter.Stop();
		}

		InternalState(const InternalState&) = delete;
		InternalState& operator=(const InternalState&) = delete;
		InternalState(InternalState&&) = delete;
		InternalState& operator=(InternalState&&) = delete;

		std::string getText()
		{
			std::scoped_lock lck(entriesLock);
			std::string retval;
			_forEachLogEntry_unsafe([&](auto&& entry) {retval += entry.text;});
			return retval;
		}
		void _forEachLogEntry_unsafe(const std::function<void(const LogEntry&)>& cb) const
		{
			for (auto& entry : entries)
			{
				cb(entry);
			}
		}
	} internal_state;

	void Flush()
	{
		asyncWriter.FlushSync();
	}

	void SetAutoFlushInterval(uint32_t milliseconds)
	{
		asyncWriter.SetAutoFlushInterval(milliseconds);
	}

	void Toggle()
	{
		enabled = !enabled;
		was_ever_enabled = true;
	}
	void Scroll(float direction)
	{
		scroll += direction;
	}
	void Update(const wi::Canvas& canvas, float dt)
	{
		if (!locked)
		{
			if (wi::input::Press(wi::input::KEYBOARD_BUTTON_HOME))
			{
				Toggle();
			}

			if (isActive())
			{
				if (wi::input::Press(wi::input::KEYBOARD_BUTTON_UP))
				{
					historyPrev();
				}
				if (wi::input::Press(wi::input::KEYBOARD_BUTTON_DOWN))
				{
					historyNext();
				}
				if (wi::input::Down(wi::input::KEYBOARD_BUTTON_PAGEUP))
				{
					Scroll(1000.0f * dt);
				}
				if (wi::input::Down(wi::input::KEYBOARD_BUTTON_PAGEDOWN))
				{
					Scroll(-1000.0f * dt);
				}

				Scroll(wi::input::GetPointer().z * 20);

				static bool created = false;
				if (!created)
				{
					created = true;
					inputField.Create("");
					inputField.SetCancelInputEnabled(false);
					inputField.OnInputAccepted([](const wi::gui::EventArgs& args) {
						historyPos = 0;
						post(args.sValue);
						LogEntry entry;
						entry.text = args.sValue;
						entry.level = LogLevel::Default;
						{
							std::scoped_lock lock(historyLock);
							history.push_back(entry);
							if (history.size() > deleteFromLine)
							{
								history.pop_front();
							}
						}
						if (!blockLuaExec)
						{
							wi::lua::RunText(args.sValue);
						}
						else
						{
							post("Lua execution is disabled", LogLevel::Error);
						}
						inputField.SetText("");
					});
					wi::Color theme_color_idle = wi::Color(30, 40, 60, 200);
					wi::Color theme_color_focus = wi::Color(70, 150, 170, 220);
					wi::Color theme_color_active = wi::Color::White();
					wi::Color theme_color_deactivating = wi::Color::lerp(theme_color_focus, wi::Color::White(), 0.5f);
					inputField.SetColor(theme_color_idle); // all states the same, it's gonna be always active anyway
					inputField.font.params.color = wi::Color(160, 240, 250, 255);
					inputField.font.params.shadowColor = wi::Color::Transparent();

					toggleButton.Create("V");
					toggleButton.OnClick([](const wi::gui::EventArgs& args) {
						Toggle();
						});
					toggleButton.SetColor(theme_color_idle, wi::gui::IDLE);
					toggleButton.SetColor(theme_color_focus, wi::gui::FOCUS);
					toggleButton.SetColor(theme_color_active, wi::gui::ACTIVE);
					toggleButton.SetColor(theme_color_deactivating, wi::gui::DEACTIVATING);
					toggleButton.SetShadowRadius(5);
					toggleButton.SetShadowColor(wi::Color(80, 140, 180, 100));
					toggleButton.font.params.color = wi::Color(160, 240, 250, 255);
					toggleButton.font.params.rotation = XM_PI;
					toggleButton.font.params.size = 24;
					toggleButton.font.params.scaling = 3;
					toggleButton.font.params.shadowColor = wi::Color::Transparent();
					for (int i = 0; i < arraysize(toggleButton.sprites); ++i)
					{
						toggleButton.sprites[i].params.enableCornerRounding();
						toggleButton.sprites[i].params.corners_rounding[2].radius = 50;
					}
				}
				if (inputField.GetState() != wi::gui::ACTIVE)
				{
					inputField.SetAsActive();
				}

			}
			else
			{
				inputField.Deactivate();
			}
		}

		if (enabled)
		{
			pos += speed * dt;
		}
		else
		{
			pos -= speed * dt;
		}
		pos = wi::math::Clamp(pos, -canvas.GetLogicalHeight(), 0);

		inputField.SetSize(XMFLOAT2(canvas.GetLogicalWidth() - 40, 20));
		inputField.SetPos(XMFLOAT2(20, canvas.GetLogicalHeight() - 40 + pos));
		inputField.Update(canvas, dt);

		toggleButton.SetSize(XMFLOAT2(100, 100));
		toggleButton.SetPos(XMFLOAT2(canvas.GetLogicalWidth() - toggleButton.GetSize().x - 20, 20 + pos));
		toggleButton.Update(canvas, dt);
	}
	void Draw(
		const wi::Canvas& canvas,
		CommandList cmd,
		ColorSpace colorspace
	)
	{
		if (!was_ever_enabled || pos <= -canvas.GetLogicalHeight())
		{
			return;
		}

		GraphicsDevice* device = GetDevice();
		device->EventBegin("Backlog", cmd);

		wi::image::Params fx = wi::image::Params((float)canvas.GetLogicalWidth(), (float)canvas.GetLogicalHeight());
		fx.pos = XMFLOAT3(0, pos, 0);
		fx.opacity = wi::math::Lerp(0.9f, 0, saturate(-pos / canvas.GetLogicalHeight()));
		if (colorspace != ColorSpace::SRGB)
		{
			fx.enableLinearOutputMapping(9);
		}
		fx.color = backgroundColor;
		wi::image::Draw(backgroundTex.IsValid() ? &backgroundTex : nullptr, fx, cmd);

		wi::image::Params inputbg;
		inputbg.color = wi::Color(80, 140, 180, 200);
		inputbg.pos = inputField.translation;
		inputbg.pos.x -= 8;
		inputbg.pos.y -= 8;
		inputbg.siz = inputField.GetSize();
		inputbg.siz.x += 16;
		inputbg.siz.y += 16;
		inputbg.enableCornerRounding();
		inputbg.corners_rounding[0].radius = 10;
		inputbg.corners_rounding[1].radius = 10;
		inputbg.corners_rounding[2].radius = 10;
		inputbg.corners_rounding[3].radius = 10;
		if (colorspace != ColorSpace::SRGB)
		{
			inputbg.enableLinearOutputMapping(9);
		}
		wi::image::Draw(nullptr, inputbg, cmd);

		if (colorspace != ColorSpace::SRGB)
		{
			inputField.sprites[inputField.GetState()].params.enableLinearOutputMapping(9);
			inputField.font.params.enableLinearOutputMapping(9);
			toggleButton.sprites[inputField.GetState()].params.enableLinearOutputMapping(9);
			toggleButton.font.params.enableLinearOutputMapping(9);
		}
		inputField.Render(canvas, cmd);

		Rect rect;
		rect.left = 0;
		rect.right = (int32_t)canvas.GetPhysicalWidth();
		rect.top = 0;
		rect.bottom = (int32_t)canvas.GetPhysicalHeight();
		device->BindScissorRects(1, &rect, cmd);

		toggleButton.Render(canvas, cmd);

		rect.bottom = int32_t(canvas.LogicalToPhysical(inputField.GetPos().y - 15));
		device->BindScissorRects(1, &rect, cmd);

		DrawOutputText(canvas, cmd, colorspace);

		rect.left = 0;
		rect.right = std::numeric_limits<int>::max();
		rect.top = 0;
		rect.bottom = std::numeric_limits<int>::max();
		device->BindScissorRects(1, &rect, cmd);
		device->EventEnd(cmd);
	}

	void DrawOutputText(
		const wi::Canvas& canvas,
		CommandList cmd,
		ColorSpace colorspace
	)
	{
		wi::font::SetCanvas(canvas); // always set here as it can be called from outside...
		wi::font::Params params = font_params;
		params.cursor = {};
		if (refitscroll.exchange(false, std::memory_order_relaxed))
		{
			const float textheight = wi::font::TextHeight(getText(), params);
			const float limit = canvas.GetLogicalHeight() - 50;
			if (scroll + textheight > limit)
			{
				scroll = limit - textheight;
			}
		}
		params.posX = 5;
		params.posY = pos + scroll;
		params.h_wrap = canvas.GetLogicalWidth() - params.posX;
		if (colorspace != ColorSpace::SRGB)
		{
			params.enableLinearOutputMapping(9);
		}

		static std::deque<LogEntry> entriesCopy;

		internal_state.entriesLock.lock();
		// Force copy because drawing text while locking is not safe because an error inside might try to lock again!
		entriesCopy = internal_state.entries;
		internal_state.entriesLock.unlock();

		for (auto& x : entriesCopy)
		{
			switch (x.level)
			{
			case LogLevel::Warning:
				params.color = wi::Color::Warning();
				break;
			case LogLevel::Error:
				params.color = wi::Color::Error();
				break;
			default:
				params.color = font_params.color;
				break;
			}
			params.cursor = wi::font::Draw(x.text, params, cmd);
		}

		unseen = LogLevel::None;
	}

	std::string getText()
	{
		return internal_state.getText();
	}
	// You generally don't want to use this. See notes in header
	void _forEachLogEntry_unsafe(const std::function<void(const LogEntry&)>& cb)
	{
		internal_state._forEachLogEntry_unsafe(cb);
	}
	void clear()
	{
		std::scoped_lock lck(internal_state.entriesLock);
		internal_state.entries.clear();
		scroll = 0;
	}
	void post(const char* input, LogLevel level)
	{
		if (logLevel > level)
		{
			return;
		}

		// Initialize async writer on first log
		if (!asyncWriter.initialized.load())
		{
			asyncWriter.Start(GetLogFile());
		}

		std::string str;
		switch (level)
		{
		default:
		case LogLevel::Default:
			str = "";
			break;
		case LogLevel::Warning:
			str = "[Warning] ";
			break;
		case LogLevel::Error:
			str = "[Error] ";
			break;
		}
		str += input;
		str += '\n';
		LogEntry entry;
		entry.text = str;
		entry.level = level;

		// Add to in-memory display buffer
		internal_state.entriesLock.lock();
		internal_state.entries.push_back(entry);
		if (internal_state.entries.size() > deleteFromLine)
		{
			internal_state.entries.pop_front();
		}
		internal_state.entriesLock.unlock();

		// Enqueue for async file writing
		asyncWriter.Enqueue(str);

		refitscroll.store(true);

		switch (level)
		{
		default:
		case LogLevel::Default:
			wi::helper::DebugOut(str, wi::helper::DebugLevel::Normal);
			break;
		case LogLevel::Warning:
			wi::helper::DebugOut(str, wi::helper::DebugLevel::Warning);
			break;
		case LogLevel::Error:
			wi::helper::DebugOut(str, wi::helper::DebugLevel::Error);
			break;
		}

		// atomic version of unseen = max(unseen, level)
		LogLevel current_unseen = unseen.load(std::memory_order_relaxed);
		while (current_unseen < level) {
			unseen.compare_exchange_weak(current_unseen, level, std::memory_order_acq_rel, std::memory_order_relaxed);
		}

		// Force an immediate flush on errors to prevent potential data loss
		//	in case the application is about to crash
		if (level >= LogLevel::Error)
		{
			asyncWriter.FlushSync();
		}
	}

	void post(const std::string& input, LogLevel level)
	{
		post(input.c_str(), level);
	}

	void historyPrev()
	{
		std::scoped_lock lock(historyLock);
		if (!history.empty())
		{
			inputField.SetText(history[history.size() - 1 - historyPos].text);
			inputField.SetAsActive();
			if ((size_t)historyPos < history.size() - 1)
			{
				historyPos++;
			}
		}
	}
	void historyNext()
	{
		std::scoped_lock lock(historyLock);
		if (!history.empty())
		{
			if (historyPos > 0)
			{
				historyPos--;
			}
			inputField.SetText(history[history.size() - 1 - historyPos].text);
			inputField.SetAsActive();
		}
	}

	void setBackground(Texture* texture)
	{
		backgroundTex = *texture;
	}
	void setBackgroundColor(wi::Color color)
	{
		backgroundColor = color;
	}
	void setFontSize(int value)
	{
		font_params.size = value;
	}
	void setFontRowspacing(float value)
	{
		font_params.spacingY = value;
	}
	void setFontColor(wi::Color color)
	{
		font_params.color = color;
	}

	bool isActive() { return enabled; }

	void Lock()
	{
		locked = true;
		enabled = false;
	}
	void Unlock()
	{
		locked = false;
	}

	void BlockLuaExecution()
	{
		blockLuaExec = true;
	}
	void UnblockLuaExecution()
	{
		blockLuaExec = false;
	}

	void SetLogLevel(LogLevel newLevel)
	{
		logLevel = newLevel;
	}

	LogLevel GetUnseenLogLevelMax()
	{
		return unseen;
	}

	void SetLogFile(const std::string& path)
	{
		logfile_path = path;
		if (asyncWriter.initialized.load())
		{
			asyncWriter.UpdateLogFilePath(GetLogFile());
		}
	}

	void GetLogFile(std::string& path)
	{
		if (!logfile_path.empty())
		{
			path = logfile_path;
		}
	}

	std::string GetLogFile()
	{
		if (logfile_path.empty() || !wi::helper::DirectoryExists(wi::helper::GetDirectoryFromPath(logfile_path)))
		{
			return wi::helper::GetCurrentPath() + "/log.txt";
		}
		return logfile_path;
	}
}
