// MM_MinimapHUDManager.c
//
// Game-mode component that registers the circular minimap InfoDisplay with
// the player's HUDManagerComponent at runtime.
//
// Usage
// -----
//   Add this component to any GameMode prefab (e.g. your custom game-mode
//   entity) or directly to the PlayerController prefab via Workbench.
//   The component detects the local player spawn and enables the minimap
//   display automatically.  No manual wiring is required beyond adding the
//   component to the prefab hierarchy.
//
// How it works
// ------------
//   On game-mode initialisation the component calls
//   SCR_HUDManagerComponent.RegisterDisplay() to inject MM_MinimapDisplay
//   into the active HUD stack.  SCR_InfoDisplay takes care of lifecycle
//   from that point (OnStartDraw / UpdateValues / OnStopDraw).

[ComponentEditorProps(category: "MinimapMod", description: "Registers the circular minimap HUD display on the local player.")]
class MM_MinimapHUDManagerClass : ScriptComponentClass {}

class MM_MinimapHUDManager : ScriptComponent
{
	//! Resource path of the InfoDisplay layout used by MM_MinimapDisplay.
	protected static const ResourceName MINIMAP_LAYOUT =
		"{5E4F2B8C9D1A3C7E}UI/layouts/MM_MinimapHUD.layout";

	//! Slot layer index – higher values render on top of lower ones.
	//! Keep this value above 0 so the minimap sits on top of the base HUD.
	protected static const int HUD_SLOT_LAYER = 50;

	// -------------------------------------------------------------------------
	// ScriptComponent lifecycle
	// -------------------------------------------------------------------------

	override void OnPostInit(IEntity owner)
	{
		super.OnPostInit(owner);

		// Only run on the client; the minimap is a purely local UI element.
		if (Replication.IsServer() && !Replication.IsClient())
			return;

		GetGame().GetCallqueue().CallLater(TryRegisterMinimap, 500, false);
	}

	// -------------------------------------------------------------------------
	// Registration
	// -------------------------------------------------------------------------

	//! Attempt to register the minimap with the local player's HUD manager.
	//! Retries every 500 ms until a player controller with a HUDManager exists.
	protected void TryRegisterMinimap()
	{
		PlayerController pc = GetGame().GetPlayerController();
		if (!pc)
		{
			// No player controller yet – retry.
			GetGame().GetCallqueue().CallLater(TryRegisterMinimap, 500, false);
			return;
		}

		SCR_HUDManagerComponent hudMgr = SCR_HUDManagerComponent.Cast(
			pc.FindComponent(SCR_HUDManagerComponent));

		if (!hudMgr)
		{
			// HUD manager not ready yet – retry.
			GetGame().GetCallqueue().CallLater(TryRegisterMinimap, 500, false);
			return;
		}

		// Register the minimap display inside the HUD stack.
		// CreateInfoDisplay creates an instance of MM_MinimapDisplay, calls
		// OnStartDraw, and begins feeding it UpdateValues every frame.
		hudMgr.CreateInfoDisplay(MINIMAP_LAYOUT);
	}
}
