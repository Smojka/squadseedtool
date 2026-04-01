// MM_MinimapDisplay.c
//
// Circular minimap HUD element for Arma Reforger.
//
// Shows a circular mini-map at the bottom-centre of the screen that renders
// the same terrain texture used by the full map (M key), so roads, mountains
// and other topographic details are visible.  The map automatically rotates
// so that the player's forward direction always points to the top.  The
// minimap is hidden whenever the local player has not yet spawned (lobby /
// dead screen) and becomes visible as soon as a controlled entity exists.
//
// Integration
// -----------
//   This class is referenced as the InfoDisplay script class inside the
//   MM_MinimapHUD.layout file.  To activate it, add the layout to your
//   player controller's HUDManagerComponent entries (see README.md).
//
// Attribute overrides
// -------------------
//   All tuneable values are exposed as [Attribute] fields so they can be
//   tweaked directly from the Workbench attribute panel without touching code.

//! Configuration container – appears in the Workbench attribute panel.
[BaseContainerProps()]
class MM_MinimapConfig : Managed
{
	//! Pixel diameter of the circular minimap widget on screen.
	[Attribute("200", UIWidgets.EditBox, "Minimap diameter (px)")]
	float m_fMinimapSize;

	//! How many metres of the world are visible across the minimap diameter.
	[Attribute("300", UIWidgets.EditBox, "Visible world radius (metres)")]
	float m_fMapRadius;

	//! Minimap background opacity (0 = fully transparent, 1 = fully opaque).
	[Attribute("0.85", UIWidgets.Slider, "Background opacity", "0 1 0.05")]
	float m_fBackgroundOpacity;

	//! When true the map texture rotates so the player always faces up.
	[Attribute("1", UIWidgets.CheckBox, "Rotate map with player heading")]
	bool m_bRotateWithPlayer;

	//! Pixel distance from the bottom edge of the screen.
	[Attribute("20", UIWidgets.EditBox, "Bottom margin (px)")]
	float m_fBottomMargin;
}

//! Main minimap InfoDisplay class.
//! Extend SCR_InfoDisplay so the engine manages lifecycle automatically.
[BaseContainerProps()]
class MM_MinimapDisplay : SCR_InfoDisplay
{
	//! How often (seconds) the minimap refreshes.  20 fps is enough for a minimap.
	protected static const float UPDATE_INTERVAL = 0.05;

	//! Tunable settings exposed in the Workbench attribute panel.
	[Attribute()]
	protected ref MM_MinimapConfig m_Config;

	// -------------------------------------------------------------------------
	// Widget references – populated from the layout in OnStartDraw
	// -------------------------------------------------------------------------
	protected FrameWidget    m_wMinimapFrame;    //!< Outer container (size / position)
	protected ImageWidget    m_wMinimapMap;      //!< The rotating terrain texture quad
	protected ImageWidget    m_wMinimapBorder;   //!< Decorative circular border overlay
	protected ImageWidget    m_wPlayerMarker;    //!< Arrow indicating player direction
	protected TextWidget     m_wCoordText;       //!< Optional grid coordinate display

	// -------------------------------------------------------------------------
	// Runtime state
	// -------------------------------------------------------------------------
	protected float  m_fTimer;          //!< Accumulates dt between updates
	protected float  m_fWorldSize;      //!< World side length in metres (square)
	protected vector m_vPlayerPos;      //!< Last known player world position
	protected float  m_fHeading;        //!< Last known player heading (degrees, 0 = north)

	// -------------------------------------------------------------------------
	// SCR_InfoDisplay overrides
	// -------------------------------------------------------------------------

	//! Called once when the HUD element is first made visible.
	override event void OnStartDraw(IEntity owner)
	{
		super.OnStartDraw(owner);
		ResolveWidgets();
		CacheWorldSize();
	}

	//! Called every frame while the HUD element is active.
	override event void UpdateValues(IEntity owner, float timeSlice)
	{
		super.UpdateValues(owner, timeSlice);

		m_fTimer += timeSlice;
		if (m_fTimer < UPDATE_INTERVAL)
			return;

		m_fTimer = 0;
		Tick();
	}

	//! Called when the HUD element is hidden.
	override event void OnStopDraw(IEntity owner)
	{
		super.OnStopDraw(owner);
	}

	// -------------------------------------------------------------------------
	// Initialisation helpers
	// -------------------------------------------------------------------------

	//! Populate widget member variables from the loaded layout tree.
	protected void ResolveWidgets()
	{
		if (!m_wRoot)
			return;

		m_wMinimapFrame  = FrameWidget.Cast(m_wRoot.FindAnyWidget("MinimapFrame"));
		m_wMinimapMap    = ImageWidget.Cast(m_wRoot.FindAnyWidget("MinimapMap"));
		m_wMinimapBorder = ImageWidget.Cast(m_wRoot.FindAnyWidget("MinimapBorder"));
		m_wPlayerMarker  = ImageWidget.Cast(m_wRoot.FindAnyWidget("PlayerMarker"));
		m_wCoordText     = TextWidget.Cast(m_wRoot.FindAnyWidget("CoordText"));
	}

	//! Cache world size once at startup; avoids repeated queries each tick.
	protected void CacheWorldSize()
	{
		BaseWorld world = GetGame().GetWorld();
		if (world)
			m_fWorldSize = world.GetWorldSize();

		// Fallback: Everon / Eden are 10 240 m squares.
		if (m_fWorldSize <= 0)
			m_fWorldSize = 10240;
	}

	// -------------------------------------------------------------------------
	// Per-tick update
	// -------------------------------------------------------------------------

	protected void Tick()
	{
		// Resolve the locally-controlled entity.
		IEntity player = GetLocalPlayer();

		// Hide when there is no controlled entity (lobby / death screen).
		if (!m_wMinimapFrame)
			return;

		if (!player)
		{
			m_wMinimapFrame.SetVisible(false);
			return;
		}

		m_wMinimapFrame.SetVisible(true);

		// Sample current player state.
		m_vPlayerPos = player.GetOrigin();
		m_fHeading   = GetPlayerHeadingDeg(player);

		RefreshMapTexture();
		RefreshPlayerMarker();
		RefreshCoordinates();
	}

	// -------------------------------------------------------------------------
	// Map texture update
	// -------------------------------------------------------------------------

	//! Update the UV rect on the terrain image so the region around the player
	//! is centred in the minimap, and rotate the image by the player's heading.
	protected void RefreshMapTexture()
	{
		if (!m_wMinimapMap || m_fWorldSize <= 0)
			return;

		// -------------------------------------------------------------------
		// UV centre – player's normalised position in [0,1]x[0,1] space.
		// Arma Reforger worlds use X/Z as horizontal axes; Y is vertical.
		// -------------------------------------------------------------------
		float uvCX = m_vPlayerPos[0] / m_fWorldSize;
		float uvCY = m_vPlayerPos[2] / m_fWorldSize;

		// Half-extent in UV space that maps to the configured map radius.
		// We show a square region that is later masked to a circle.
		float mapRadius = (m_Config) ? m_Config.m_fMapRadius : 300.0;
		float uvHalf = mapRadius / m_fWorldSize;

		float uvL = Math.ClampFloat(uvCX - uvHalf, 0.0, 1.0);
		float uvT = Math.ClampFloat(uvCY - uvHalf, 0.0, 1.0);
		float uvW = Math.ClampFloat(uvCX + uvHalf, 0.0, 1.0) - uvL;
		float uvH = Math.ClampFloat(uvCY + uvHalf, 0.0, 1.0) - uvT;

		m_wMinimapMap.SetUVRect(uvL, uvT, uvW, uvH);

		// Rotate the image quad so the player's forward direction faces up.
		WidgetTransform xf = new WidgetTransform();
		m_wMinimapMap.GetTransform(xf);
		xf.m_fAngle = -m_fHeading;
		m_wMinimapMap.SetTransform(xf);
	}

	// -------------------------------------------------------------------------
	// Player marker update
	// -------------------------------------------------------------------------

	//! Keep the arrow marker centred and always pointing up (north relative to
	//! the rotated map underneath it).
	protected void RefreshPlayerMarker()
	{
		if (!m_wPlayerMarker)
			return;

		// The map rotates, but the player marker always faces up.
		WidgetTransform xf = new WidgetTransform();
		m_wPlayerMarker.GetTransform(xf);
		xf.m_fAngle = 0;
		m_wPlayerMarker.SetTransform(xf);
	}

	// -------------------------------------------------------------------------
	// Coordinate text update
	// -------------------------------------------------------------------------

	//! Display grid coordinates (metres from world origin, rounded to 10 m).
	protected void RefreshCoordinates()
	{
		if (!m_wCoordText)
			return;

		int gx = (int)(m_vPlayerPos[0] / 100.0);
		int gy = (int)(m_vPlayerPos[2] / 100.0);
		m_wCoordText.SetText(string.Format("%1-%2", gx, gy));
	}

	// -------------------------------------------------------------------------
	// Utility helpers
	// -------------------------------------------------------------------------

	//! Return the local player's controlled entity, or null if none.
	protected IEntity GetLocalPlayer()
	{
		PlayerController pc = GetGame().GetPlayerController();
		if (!pc)
			return null;

		return pc.GetControlledEntity();
	}

	//! Return the player's horizontal heading in degrees (0 = north / +Z axis).
	protected float GetPlayerHeadingDeg(IEntity player)
	{
		if (!player)
			return 0;

		// Extract forward vector from entity's world-space transform.
		vector mat[4];
		player.GetTransform(mat);

		// mat[2] is the forward (Z) basis vector.
		// Atan2(forward.x, forward.z) gives azimuth from +Z north.
		return Math.Atan2(mat[2][0], mat[2][2]) * Math.RAD2DEG;
	}
}
