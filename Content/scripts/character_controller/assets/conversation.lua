ConversationState = {
	Disabled = 0,
	Talking = 1,
	Waiting = 2,
}

function Conversation()
	local self = {
		state = ConversationState.Disabled,
		percent = 0,
		character = nil,
		font = SpriteFont(),
		advance_font = SpriteFont(""),
		choice_fonts = {},
		font_blink_timer = 0,
		dialog = {},
		speed = 30,
		choice = 1,
		override_input = false,

		Update = function(self, path, scene, player)

			local crop_top = GetScreenHeight() * 0.17
			local crop_bottom = GetScreenHeight() * 0.21
			local text_offset = GetScreenWidth() * 0.2
			self.font.SetPos(Vector(text_offset, GetScreenHeight() - crop_bottom + 10))
			self.font.SetHorizontalWrapping(GetScreenWidth() - self.font.GetPos().GetX() * 2)
			self.advance_font.SetPos(Vector(GetScreenWidth() - self.font.GetPos().GetX(), GetScreenHeight() - 50))

			-- Update conversation percentage (fade in/out of conversation)
			if self.state == ConversationState.Disabled then
				self.percent = math.lerp(self.percent, 0, getDeltaTime() * 4)
			else
				self.percent = math.lerp(self.percent, 1, getDeltaTime() * 4)
			end
			path.SetCropTop(self.percent * crop_top)
			path.SetCropBottom(self.percent * crop_bottom)
			local cam = GetCamera()
			cam.SetApertureSize(self.percent)

			if self.percent < 0.9 then
				self.font.SetHidden(true)
				self.font.ResetTypewriter()
			else
				self.font.SetHidden(false)
			end
			self.advance_font.SetColor(Vector())
			self.override_input = false
			for i,choice_font in ipairs(self.choice_fonts) do
				choice_font.SetColor(Vector())
			end
			self.font_blink_timer = self.font_blink_timer + getDeltaTime()

			-- Focus on character:
			if self.character ~= nil then
				cam.SetFocalLength(vector.Subtract(scene.Component_GetTransform(self.character.head).GetPosition(), cam.GetPosition()).Length())
			end

			-- State flow:
			if self.state == ConversationState.Disabled then
				self.font.SetHidden(true)
			elseif self.state == ConversationState.Talking then

				self.choice = 1
				self.override_input = true
				self:CinematicCamera(self.character, player, scene, cam)

				-- End of talking:
				if self.font.IsTypewriterFinished() then
					self.state = ConversationState.Waiting
					self.font_blink_timer = 0
				end

				-- Skip talking:
				if input.Press(KEYBOARD_BUTTON_ENTER) or input.Press(KEYBOARD_BUTTON_SPACE) or input.Press(GAMEPAD_BUTTON_2) then
					self.font.TypewriterFinish()
				end

				-- Turn on talking animation:
				if self.character.expression ~= INVALID_ENTITY and self.percent >= 0.9 then
					scene.Component_GetExpression(self.character.expression).SetForceTalkingEnabled(true)
				end

			elseif self.state == ConversationState.Waiting then

				-- End of talking, waiting for input:
				
				if self.dialog.choices ~= nil then
					-- Dialog choices:
					self.override_input = true
					self:CinematicCamera(player, self.character, scene, cam)

					local pos = vector.Add(self.font.GetPos(), Vector(20, 10 + self.font.TextSize().GetY()))
					for i,choice in ipairs(self.dialog.choices) do
						if self.choice_fonts[i] == nil then
							self.choice_fonts[i] = SpriteFont()
							path.AddFont(self.choice_fonts[i])
						end
						self.choice_fonts[i].SetPos(pos)
						self.choice_fonts[i].SetSize(self.font.GetSize())
						self.choice_fonts[i].SetHorizontalWrapping(GetScreenWidth() - self.choice_fonts[i].GetPos().GetX() * 2)
						if i == self.choice then
							self.choice_fonts[i].SetText(" " .. choice[1])
							self.choice_fonts[i].SetColor(vector.Lerp(Vector(1,1,1,1), self.font.GetColor(), math.abs(math.sin(self.font_blink_timer * math.pi))))
						else
							self.choice_fonts[i].SetText("     " .. choice[1])
							self.choice_fonts[i].SetColor(self.font.GetColor())
						end
						pos = vector.Add(pos, Vector(0, self.choice_fonts[i].TextSize().GetY() + 5))
					end

					-- Dialog input:
					if input.Press(KEYBOARD_BUTTON_UP) or input.Press(string.byte('W')) or input.Press(GAMEPAD_BUTTON_UP) then
						self.choice = self.choice - 1
						self.font_blink_timer = 0
					end
					if input.Press(KEYBOARD_BUTTON_DOWN) or input.Press(string.byte('S')) or input.Press(GAMEPAD_BUTTON_DOWN) then
						self.choice = self.choice + 1
						self.font_blink_timer = 0
					end
					if self.choice < 1 then
						self.choice = #self.dialog.choices
					end
					if self.choice > #self.dialog.choices then
						self.choice = 1
					end
					if input.Press(KEYBOARD_BUTTON_ENTER) or input.Press(KEYBOARD_BUTTON_SPACE) or input.Press(GAMEPAD_BUTTON_2) then
						if self.dialog.choices[self.choice].action ~= nil then
							self.dialog.choices[self.choice].action() -- execute dialog choice action
						end
						self:Next()
					end
				else
					-- No dialog choices:
					self.override_input = true
					self:CinematicCamera(self.character, player, scene, cam)

					if input.Press(KEYBOARD_BUTTON_ENTER) or input.Press(KEYBOARD_BUTTON_SPACE) or input.Press(GAMEPAD_BUTTON_2) then
						if self.dialog.action_after ~= nil then
							self.dialog.action_after() -- execute dialog action after ended
						end
						self:Next()
						self.font.ResetTypewriter()
					end
					-- Blinking advance conversation icon:
					self.advance_font.SetColor(vector.Lerp(Vector(0,0,0,0), self.font.GetColor(), math.abs(math.sin(self.font_blink_timer * math.pi))))	
				end

				-- Turn off talking animation:
				if self.character.expression ~= INVALID_ENTITY then
					scene.Component_GetExpression(self.character.expression).SetForceTalkingEnabled(false)
				end
			end

		end,
		
		Enter = function(self, character)
			backlog_post("Entering conversation")
			self.state = ConversationState.Talking
			self.character = character
			if #self.character.dialogs < self.character.next_dialog then
				self.character.next_dialog = 1
			end
			self:Next()
		end,
	
		Exit = function(self)
			backlog_post("Exiting conversation")
			self.state = ConversationState.Disabled
			self.dialog = {}
			self.override_input = false
			self.font.ResetTypewriter()
		end,

		Next = function(self)
			if #self.character.dialogs < self.character.next_dialog then
				self:Exit()
			end
			if self.state == ConversationState.Disabled then
				return
			end
			self.dialog = self.character.dialogs[self.character.next_dialog]
			self.character.next_dialog = self.character.next_dialog + 1
			self.state = ConversationState.Talking
			self.font.SetText(self.dialog[1])
			self.font.SetTypewriterTime(string.len(self.dialog[1] or "") / self.speed)
			self.font.ResetTypewriter()
			if self.dialog.action ~= nil then
				self.dialog.action() -- execute dialog action
			end
		end,

		CinematicCamera = function(self, character_foreground, character_background, scene, camera)
			local head_pos = scene.Component_GetTransform(character_foreground.head).GetPosition()
			local forward_dir = vector.Subtract(character_background.position, character_foreground.position).Normalize()
			forward_dir = vector.TransformNormal(forward_dir, matrix.RotationY(math.pi * 0.1))
			local up_dir = Vector(0,1,0)
			local right_dir = vector.Cross(up_dir, forward_dir).Normalize()
			local cam_pos = vector.Add(head_pos, forward_dir)
			local cam_target = vector.Add(vector.Add(head_pos, vector.Multiply(right_dir, -0.4)), Vector(0,-0.1))
			local lookat = matrix.Inverse(matrix.LookAt(cam_pos, cam_target))
			camera.TransformCamera(lookat)
			camera.UpdateCamera()
			camera.SetFocalLength(vector.Subtract(head_pos, camera.GetPosition()).Length())
		end
	}
	self.font.SetSize(20)
	self.font.SetColor(Vector(0.6,0.8,1,1))
	self.advance_font.SetSize(24)
	local sound = Sound()
	local soundinstance = SoundInstance()
	audio.CreateSound(script_dir() .. "typewriter.wav", sound)
	audio.CreateSoundInstance(sound, soundinstance)
	audio.SetVolume(0.2, soundinstance)
	self.font.SetTypewriterSound(sound, soundinstance)
	return self
end
