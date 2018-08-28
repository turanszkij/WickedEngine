#include "wiGUI.h"
#include "wiWidget.h"
#include "wiHashString.h"
#include "wiRenderer.h"
#include "wiInputManager.h"

using namespace std;

wiGUI::wiGUI(GRAPHICSTHREAD threadID) :threadID(threadID), activeWidget(nullptr), focus(false), visible(true), pointerpos(XMFLOAT2(0,0))
{
}


wiGUI::~wiGUI()
{
}


void wiGUI::Update(float dt)
{
	if (!visible)
	{
		return;
	}

	for (size_t i = 0; i < wiWidget::transforms.GetCount(); ++i)
	{
		auto& transform = wiWidget::transforms[i];

		const bool parented = wiWidget::transforms.IsValid(transform.parent_ref);

		if (transform.dirty || parented)
		{
			transform.dirty = false;

			XMVECTOR scale_local = XMLoadFloat3(&transform.scale_local);
			XMVECTOR rotation_local = XMLoadFloat4(&transform.rotation_local);
			XMVECTOR translation_local = XMLoadFloat3(&transform.translation_local);
			XMMATRIX world =
				XMMatrixScalingFromVector(scale_local) *
				XMMatrixRotationQuaternion(rotation_local) *
				XMMatrixTranslationFromVector(translation_local);

			if (parented)
			{
				auto& parent = wiWidget::transforms.GetComponent(transform.parent_ref);
				XMMATRIX world_parent = XMLoadFloat4x4(&parent.world);
				XMMATRIX bindMatrix = XMLoadFloat4x4(&transform.world_parent_bind);
				world = world * bindMatrix * world_parent;
			}

			XMVECTOR S, R, T;
			XMMatrixDecompose(&S, &R, &T, world);
			XMStoreFloat3(&transform.scale, S);
			XMStoreFloat4(&transform.rotation, R);
			XMStoreFloat3(&transform.translation, T);

			transform.world_prev = transform.world;
			XMStoreFloat4x4(&transform.world, world);
		}

	}

	XMFLOAT4 _p = wiInputManager::GetInstance()->getpointer();
	pointerpos.x = _p.x;
	pointerpos.y = _p.y;

	if (activeWidget != nullptr)
	{
		if (!activeWidget->IsEnabled() || !activeWidget->IsVisible())
		{
			// deactivate active widget if it became invisible or disabled
			DeactivateWidget(activeWidget);
		}
	}

	focus = false;
	for (list<wiWidget*>::reverse_iterator it = widgets.rbegin(); it != widgets.rend(); ++it)
	{
		if ((*it)->container == nullptr)
		{
			// the contained child widgets will be updated by the containers
			(*it)->Update(this, dt);
		}

		if ((*it)->IsEnabled() && (*it)->IsVisible() && (*it)->GetState() > wiWidget::WIDGETSTATE::IDLE)
		{
			focus = true;
		}
	}
}

void wiGUI::Render()
{
	if (!visible)
	{
		return;
	}

	wiRenderer::GetDevice()->EventBegin("GUI", GetGraphicsThread());
	for (auto&x : widgets)
	{
		if (x->container == nullptr && x != activeWidget)
		{
			// the contained child widgets will be rendered by the containers
			x->Render(this);
		}
	}
	if (activeWidget != nullptr)
	{
		// render the active widget on top of everything
		activeWidget->Render(this);
	}

	for (auto&x : widgets)
	{
		x->RenderTooltip(this);
	}

	ResetScissor();
	wiRenderer::GetDevice()->EventEnd(GetGraphicsThread());
}

void wiGUI::ResetScissor()
{
	wiGraphicsTypes::Rect scissor[1];
	scissor[0].bottom = (LONG)(wiRenderer::GetDevice()->GetScreenHeight());
	scissor[0].left = (LONG)(0);
	scissor[0].right = (LONG)(wiRenderer::GetDevice()->GetScreenWidth());
	scissor[0].top = (LONG)(0);
	wiRenderer::GetDevice()->BindScissorRects(1, scissor, GetGraphicsThread());
}

void wiGUI::AddWidget(wiWidget* widget)
{
	widgets.push_back(widget);
}

void wiGUI::RemoveWidget(wiWidget* widget)
{
	widgets.remove(widget);
}

wiWidget* wiGUI::GetWidget(const wiHashString& name)
{
	for (auto& x : widgets)
	{
		if (x->GetName() == name)
		{
			return x;
		}
	}
	return nullptr;
}

void wiGUI::ActivateWidget(wiWidget* widget)
{
	activeWidget = widget;
	activeWidget->Activate();
}
void wiGUI::DeactivateWidget(wiWidget* widget)
{
	widget->Deactivate();
	if (activeWidget == widget)
	{
		activeWidget = nullptr;
	}
}
wiWidget* wiGUI::GetActiveWidget()
{
	return activeWidget;
}
bool wiGUI::IsWidgetDisabled(wiWidget* widget)
{
	return (activeWidget != nullptr && activeWidget != widget);
}
bool wiGUI::HasFocus()
{
	if (!visible)
	{
		return false;
	}

	return focus;
}
