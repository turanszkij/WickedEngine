#ifndef WI_SHADERINTEROP_SURFEL_GI_H
#define WI_SHADERINTEROP_SURFEL_GI_H
#include "ShaderInterop.h"
#include "ShaderInterop_Renderer.h"

static const uint SURFEL_CAPACITY = 100000;
static const uint SQRT_SURFEL_CAPACITY = (uint)ceil(sqrt((float)SURFEL_CAPACITY));
static const uint SURFEL_MOMENT_RESOLUTION = 4;
static const uint SURFEL_MOMENT_ATLAS_TEXELS = SQRT_SURFEL_CAPACITY * SURFEL_MOMENT_RESOLUTION;
static const uint3 SURFEL_GRID_DIMENSIONS = uint3(128, 64, 128);
static const uint SURFEL_TABLE_SIZE = SURFEL_GRID_DIMENSIONS.x * SURFEL_GRID_DIMENSIONS.y * SURFEL_GRID_DIMENSIONS.z;
static const uint SURFEL_GRID_LEVELS = 8; // cascaded grid levels; level L has cell/radius SURFEL_MIN_RADIUS << L (3, 6, 12, 24, 48, 96, 192, 384). More levels than the near field needs so distant surfels can grow large (few, coarse) via SURFEL_RADIUS_GROWTH_DISTANCE instead of saturating at a small size
static const uint SURFEL_TOTAL_TABLE_SIZE = SURFEL_TABLE_SIZE * SURFEL_GRID_LEVELS; // grid cells across all levels
static const float SURFEL_MIN_RADIUS = 3.0; // level-0 (finest) cell size and radius; near surfaces reach this, so surfel density keeps rising as the camera approaches (no hard floor at 2 like before)
static const float SURFEL_MAX_RADIUS = SURFEL_MIN_RADIUS * (float)(1u << (SURFEL_GRID_LEVELS - 1)); // derived coarsest radius (level LEVELS-1); used as a default/liveness radius value
static const float SURFEL_RADIUS_PIXELS = 32; // target screen-space radius in pixels that drives which level a surfel lands on
static const float SURFEL_RADIUS_GROWTH_DISTANCE = 50; // distance (world units) over which the surfel size grows SUPER-linearly with distance in surfel_level: the screen-footprint target is scaled by (1 + dist / this), so the nearest surfels keep their current fine size (scale ~1) while far ones coarsen far faster than a constant footprint would - a distant surface then needs far fewer, bigger surfels, slashing the far working set (gather + re-trace + live count) that tanks FPS when a large area comes into view (flying up). Smaller = more aggressive far growth (fewer/coarser far surfels); very large = disables the growth (back to a constant screen footprint)
static const float SURFEL_LEVEL_DITHER = 1.0f; // per-surfel spread (in cascade levels) of the level-change threshold in surfel_stable_level (surfel_update). A moving camera puts a flat region all at ~one distance, so WITHOUT this every surfel there crosses a cascade-level boundary on the SAME frame - re-binning, growing, going redundant and mass thin+respawning together, which POPS. Dithering each surfel's threshold by a stable per-surfel amount spreads those transitions across a band of distance so they trickle a few at a time instead of storming. 0 = no spread (all cross together, the old behaviour); 1 = spread over a full level width. A fixed hysteresis dead-band (see surfel_stable_level) additionally stops a surfel hovering at a boundary from flip-flopping its level frame to frame
// Relevance-based recycler (see surfel_updateCS): the live working set is bounded
// by recycling the least-relevant surfels probabilistically. Relevance falls with
// recency (frames since a surfel last contributed to a visible pixel), distance
// from the camera, and pool pressure (live count vs. the soft target below).
static const uint SURFEL_LIVE_TARGET = 80000; // soft cap on live surfels; eviction ramps as the live count approaches/exceeds this (kept well below SURFEL_CAPACITY so recycling actually engages)
static const uint SURFEL_RECYCLE_RECENCY_MIN = 32; // frames a surfel must go unseen before it becomes eligible for recency-based eviction
static const uint SURFEL_RECYCLE_RECENCY_MAX = 200; // frames unseen at which recency eviction saturates (must stay < 256: recycle is packed at 8 bits)
static const float SURFEL_RECYCLE_DISTANCE_FAR = 200; // distance from camera (world units) at which the distance eviction bias saturates
static const float SURFEL_RECYCLE_DISTANCE_BOOST = 3; // max multiplier on eviction probability for the most distant surfels (near surfels keep 1x); only biases which already-evictable surfels go first
static const float SURFEL_RECYCLE_PRESSURE_FLOOR = 0.02f; // staleness-trickle eviction scale for fully-stale surfels (recency 1); recency-gated (a surfel seen this frame is never recycled) AND fill-gated by SURFEL_RECYCLE_TRICKLE_START below
static const float SURFEL_RECYCLE_TRICKLE_START = 0.75f; // pool fill fraction (of SURFEL_LIVE_TARGET) at which the staleness trickle STARTS reclaiming long-unseen surfels. Below this the pool has ample headroom, so off-screen surfels are KEPT - world-space GI persists when you pan a full surface off screen and back, instead of the whole cache draining just because it's not visible. From here to the target the trickle ramps in to make room before the hard overflow shed engages; over target the overflow shed (below) bounds the set. Lower = reclaim sooner (less persistence); 1.0 = never trickle, rely purely on the overflow shed at/over target
static const float SURFEL_RECYCLE_OVERFLOW_GAIN = 0.25f; // over-target shed: per-frame eviction scale once past the soft target, applied regardless of recency so an all-visible view (e.g. sky looking down) can't grow the set without bound
static const uint SURFEL_PROPERTY_SEEN_BIT = 1u << 17u; // SurfelData.properties bit set by surfel_coverageCS when a surfel contributes to a visible pixel
static const uint SURFEL_INDIRECT_NUMTHREADS = 32;
static const float SURFEL_TARGET_COVERAGE = 0.8f; // how many surfels should affect a pixel fully, higher values will increase quality and cost
static const float SURFEL_COVERAGE_TEMPORAL_BLEND = 0.2f; // fraction of THIS frame's freshly gathered GI blended into the temporal history each frame in surfel_coverage (the rest is carried from the reprojected history) when history is CONSISTENT with the fresh value. Lower = more temporal smoothing/denoising and more history reuse, but slower to react to lighting changes; 1 = no accumulation (history ignored). The history is reprojected by the motion vector, so this denoises a stable view without ghosting under camera motion
static const float SURFEL_COVERAGE_TEMPORAL_REJECT = 4.0f; // consistency sensitivity for the temporal blend. The reprojected history's blend weight ramps back toward fresh (alpha->1) as its relative luminance difference from the fresh gather times this exceeds 1 - so a disocclusion / lighting change / reprojection error (history disagrees) falls back to the freshly gathered value instead of ghosting. Higher = reject history sooner (less ghosting, less denoising); lower = hold history through bigger differences (more denoising, more ghost risk)
static const float SURFEL_COVERAGE_DISOCCLUSION = 0.05f; // relative linear-depth tolerance for the temporal reprojection. If the previous-frame depth at the reprojected location differs from this pixel's depth by more than this fraction, the pixel was occluded last frame (a behind-object reveal), so its reprojected history is a DIFFERENT surface - reject it. For a skipped group this forces the whole group to refresh (a full gather) instead of ghosting the occluder's GI onto the revealed surface; for a refreshed group it just takes the fresh value. Lower = stricter (catches smaller disocclusions but may false-trigger a refresh on fast dolly, costing some speed); higher = looser (more speed, more residual ghosting at reveals)
static const float SURFEL_COVERAGE_SKIP_MIN_DEPTH = 100.0f; // linear depth (world units) below which a coverage group is NEVER skipped - it always does a full gather. Near geometry has high screen parallax for a given camera move, so the reprojected-history reuse of the gather-skip smears visibly there (e.g. a small close scene like a Cornell box ghosts on a slight camera move); it's also the cheap case, so skipping buys little. Beyond this depth the parallax is small (ghost hidden) and it's the expensive far case worth skipping - so the skip engages only where it pays and doesn't show. A group with ANY pixel nearer than this refreshes wholesale. Lower = skip closer (more speed, more near ghost risk); very large = disable the skip everywhere (always gather); 0 = allow the skip at any depth
static const uint SURFEL_COVERAGE_REFRESH_PERIOD = 8; // temporal gather-SKIP cadence: only 1/this of the coverage thread groups do a full surfel gather each frame (a rotating set that sweeps the screen over this many frames); the rest reuse their reprojected history and skip the (expensive) gather + spawn entirely - this is the actual coverage speedup. A group is also forced to refresh if any of its pixels reproject off-screen (screen-edge disocclusion) or when a debug view is active. MUST stay below the recycler's unseen window (SURFEL_RECYCLE_RECENCY_MIN): a skipped group doesn't mark its surfels seen, so they're only refreshed on their group's cadence frames; keeping the period under that window means their unseen counter never climbs enough to evict them. Higher = cheaper coverage but more temporal lag / staler history between refreshes; 1 = disable the skip (gather every group every frame)
static const float SURFEL_NORMAL_WEIGHT_POWER = 2; // exponent sharpening the normal-similarity term in the GI lookup; keep mild (the tangent-plane term is the real anti-leak) - too high collapses the contributor count on curved/foliage surfaces, which under-averages and lets GI fireflies show through
static const float SURFEL_PLANE_WEIGHT_SCALE = 0.7f; // tangent-plane tolerance as a fraction of radius; smaller = stricter anti-leak (a surfel only affects a thin slab around its own plane, so it can't leak onto a perpendicular surface across an edge)
static const float SURFEL_SPAWN_MIN_SPACING = 0.2f; // THE NEAR DENSITY KNOB. Poisson-disk spacing as a fraction of radius at the FINEST level (level 0, closest surfaces): the spawner fills until every point has a same-layer, same-orientation neighbour within radius*this, then stops (nearest neighbour comes from the coverage gather). Smaller = denser / more overlap near the camera. Coarser (more distant) levels interpolate toward SURFEL_SPAWN_MAX_SPACING via surfel_spawn_spacing(), so far surfels are sparser WITHOUT changing this near density. Placement is driven by THIS, not by coverage (big surfels saturate coverage while still sparse, which stalled the fill)
static const float SURFEL_SPAWN_MAX_SPACING = 0.5f; // THE FAR DENSITY KNOB. Poisson-disk spacing fraction at the COARSEST level (SURFEL_GRID_LEVELS-1, most distant surfaces). surfel_spawn_spacing() lerps by level from SURFEL_SPAWN_MIN_SPACING (near, dense) to this (far, sparse), so distant surfels spread out further than the near-field spacing while the closest level keeps its density. Must be >= MIN; set equal to MIN to disable the distance falloff (uniform spacing at every level). This spacing is a fraction of the (already distance-grown) radius, so far surfels get sparser both from their larger radius AND this larger fraction
static const float SURFEL_SPAWN_PER_CELL = 3.0f; // target EXPECTED new surfels per world cell per frame. The spawner is screen-space (one candidate per tile), so up close many tiles overlap one world cell and would all spawn into it at once (the burst/clump). Each tile's spawn probability is scaled by (tile_world_size/cell_size)^2 (i.e. by distance^2) so the expected spawns per cell per frame stays ~this regardless of camera distance - close surfaces no longer over-spawn. Higher = faster fill but burstier; lower = gentler/more uniform
static const float SURFEL_THIN_HYSTERESIS = 0.75f; // thinning distance as a fraction of the spawn spacing. surfel_integrate marks a surfel redundant when a same-orientation, LOWER-INDEX neighbour sits within radius * SURFEL_SPAWN_MIN_SPACING * this; kept < 1 so a just-spawned surfel (which needed an empty spawn-spacing radius) is not instantly re-thinned - the gap prevents spawn/thin oscillation. This is the "discard on recede" half of EA's scheme: as surfels grow when the camera backs away they become over-dense, and the excess is recycled
static const float SURFEL_THIN_RATE = 0.1f; // per-frame probability that surfel_update recycles a surfel flagged redundant. Gradual (not instant) so an over-dense region relaxes to the spacing target smoothly instead of collapsing in one frame
static const uint SURFEL_CELL_LIMIT = 512; // hard per-cell ceiling: surfel_coverage stops spawning into a (fine-level) grid cell once it already holds this many surfels, and the per-pixel GI gather loops a cell's surfels, so this bounds both worst-case density and gather cost. SURFEL_SPAWN_MIN_SPACING is the PRIMARY density control (the Poisson spacing gate); this is the safety ceiling above it. Note the count includes surfels OVERLAPPING the cell from neighbours (each surfel bins into up to 27 cells), so setting it too low starves cells via neighbour-bleed - an empty cell reads a high count purely from its full neighbours and never spawns, leaving grid-aligned empty squares. Keep it comfortably above the surfels-per-cell that SURFEL_SPAWN_MIN_SPACING implies
static const uint SURFEL_SPAWN_BUDGET = 8192; // safety cap on new surfels per frame (spawnCount atomically counted, reset each frame). With the distance-compensated per-cell rate above doing the real density control, this is just a ceiling for pathological views; lower it if you also want a deliberately slower reveal
static const float SURFEL_SPAWN_FADE_FRAMES = 64; // frames over which a newborn surfel's GI contribution WEIGHT ramps from 0 to full in surfel_coverage (by its life counter). A just-spawned surfel can start off-colour - dark if its first rays were ray-budget-starved, or bright from a stray ray - before the estimator converges to its neighbours; snapping it straight to full weight at life 1 makes that error POP. Fading the weight in lets it blend and converge first, so spawns are not visible. Higher = smoother/slower reveal (a surface lights up more gradually); 1 = the old hard 1-frame snap
static const float SURFEL_EMISSIVE_SPAWN_SKIP = 2.0f; // do not spawn surfels on strongly emissive surfaces (max emissive channel >= this). Their own look is dominated by emission, so cached diffuse GI on them is wasted; they still act as GI light sources for other surfels via ray hits (emission is picked up at ray-trace hit points, not from surfels sitting on them), so skipping placement here costs nothing in light transport. 0 disables the skip
static const float SURFEL_METALLIC_ALBEDO_SKIP = 0.001f; // do not spawn surfels on fully-metallic surfaces (max albedo channel <= this). Surface stores no metalness, but full metalness zeroes albedo (albedo = baseColor * (1 - max(reflectance, metalness)) => 0 at metalness 1), so a ~zero albedo is exactly the "metalness >= 1" case. Metals have no diffuse response, so cached diffuse GI is invisible on them (they reflect via specular/RT reflections, not surfels); this also skips fully-black diffuse, which likewise shows no GI. Raise slightly to also skip near-metals; set < 0 to disable
static const float SURFEL_TRANSMISSION_SPAWN_SKIP = 0.0f; // do not spawn surfels on transparent surfaces (surface.transmission > this). transmission already folds in cloak (transmission = lerp(GetTransmission(), 1, GetCloak())), so this covers both transmissive and cloaked materials. Glass/transparent surfaces transmit and refract light along ray paths rather than reflecting it diffusely, so cached diffuse GI on them is meaningless. Default 0 skips any transparent surface; raise to require more transmission before skipping; set >= 1 to disable (transmission maxes at 1)
static const uint SURFEL_RAY_BUDGET = 100000; // max number of rays per frame
static const uint SURFEL_RAY_BOOST_MAX = 64; // max rays per surfel, at the FINEST level (level 0, near). surfel_update scales the per-surfel ray boost down with cascade level (toward SURFEL_RAY_BOOST_MIN at the coarsest level), so near/detailed surfels get the full ray count while coarse far ones - which are low-frequency and long-lived, so they converge fine on fewer rays per frame via temporal accumulation - don't saturate the ray budget when a huge far area is visible at once (looking down from altitude). Packed at 8 bits, so must stay < 256
static const uint SURFEL_RAY_BOOST_MIN = 8; // rays per surfel at the COARSEST level (most distant surfaces). The boost lerps SURFEL_RAY_BOOST_MAX -> this by level, so the far field is refreshed with far fewer rays; its per-frame estimate is noisier but the temporal estimator averages it out over the surfel's (long) life, so steady-state quality is unchanged while the worst-case ray count (bird's-eye view) drops sharply. Set equal to MAX to disable the per-level ray falloff (uniform ray count)
static const uint SURFEL_RAY_UPDATE_PERIOD_MAX = 8; // TEMPORAL RAY AMORTIZATION. Frames between ray re-traces for a surfel at the COARSEST cascade level (most distant). surfel_update gives each surfel a re-trace period that grows with its continuous distance level - 1 (every frame) near, ramping to this at the coarsest cascade level - and skips ray allocation entirely on the frames that aren't its turn; a per-surfel phase staggers the turns so a whole region doesn't re-trace on the same frame (which would pulse). This attacks the raytrace cost directly: when a large far area comes into view (flying up over terrain) only ~1/period of the far surfels request rays each frame, so the per-frame ray count for the far field drops by up to this factor. Safe because distant surfels are low-frequency, long-lived, and faded in over SURFEL_SPAWN_FADE_FRAMES(64) frames, so re-tracing them a fraction as often still converges via the temporal estimator with no visible popping; the ray count each surfel gets ON its turn is unchanged, only the cadence drops. Near/detailed surfels keep period 1 (every frame). Higher = fewer far rays / cheaper but slower far convergence; 1 = disable (every surfel re-traces every frame, the old behaviour)
static const uint SURFEL_RAY_UPDATE_PERIOD_CAP = 32; // upper clamp on the amortization period for surfels FAR BEYOND the coarsest cascade level. The cascade level saturates at SURFEL_GRID_LEVELS-1 (radius capped), so a bird's-eye view puts a huge area of surfels all at the top level - but by DISTANCE they can be many times further than that level's threshold, and those need refreshing least of all. surfel_update drives the period from the surfel's CONTINUOUS (unclamped) distance level, so past the top cascade level the period keeps doubling per extra level of distance, clamped here. This is what extends the amortization into the worst case (flying very high, everything saturated at the coarsest level). Must be >= SURFEL_RAY_UPDATE_PERIOD_MAX; set equal to it to disable the beyond-top-level extension (period stops growing at the coarsest cascade level). Keep well under the coverage seen-window so far surfels are still re-seen/kept; convergence at the cap is very slow but the fade-in + long life hide it
static const float SURFEL_RAY_GUIDE_FRACTION = 0.5f; // max fraction of bounce rays steered toward the surfel's brightest cached direction (scaled by how directional it is)
static const float SURFEL_RAY_RADIANCE_MAX = 16000.0f; // hard clamp on a ray's returned radiance before it is stored/packed (surfel_raytraceCS). A ray hitting the sky/sun (physical sky sun disk) returns radiance far above the R11G11B10 pack range (~65504), which unpacks as +Inf; MultiscaleMeanEstimator's firefly clamp then does Inf - Inf = NaN, poisoning the surfel's cached radiance (and spreading via the multi-bounce feedback), which the coverage gather reads as NaN and flashes the whole grid cell black/white. Clamping to a finite value the pack format represents keeps Inf/NaN out of the cache at the source; it also doubles as a firefly ceiling (lower = less GI variance/brightness, but clips very bright direct-sky bounces). Must stay well below the R11G11B10 max
static const float SURFEL_RAY_SORT_CELL = 4.0f; // world-space cell size (units) for the ray-sort Morton key: the surfel position is quantised to this before Morton coding, so rays from surfels within ~this distance share a key prefix and trace coherently. Smaller = finer spatial buckets (more precise coherence, but the 30-bit Morton wraps over a smaller world extent); larger = coarser buckets. Only used when SURFEL_RAY_SORTING is defined
// Edge-aware a-trous denoiser (surfel_denoiseCS), run on the HALF-RES GI output
// between coverage and the bilateral upsample. The remaining flat-surface
// blotchiness is per-surfel radiance variance (not density); temporal
// accumulation alone can't remove it, so a downstream spatial filter of the
// final gather is the SOTA fix (see Tomasz Stachowiak / kajiya). This runs
// OUTPUT-side (reads/writes the GI result textures, never the surfel cache), so
// it cannot feed back and run away the way spatial sharing INSIDE the cache
// does. The temporal history is left un-denoised (only a copy sent to the
// upsample is filtered), so the spatial blur never compounds through the
// temporal loop.
static const uint SURFEL_DENOISE_PASSES = 3; // a-trous iterations on the half-res GI. Each pass doubles the tap stride (1,2,4,...), so N passes smooth over a ~2^N-tap footprint while touching only 25 taps each. More = wider smoothing of the blotches (and more cost); 0 disables the denoiser entirely (straight coverage -> upsample, the old path)
static const float SURFEL_DENOISE_NORMAL_POWER = 32.0f; // edge-stop sharpness on normal similarity: tap weight *= pow(saturate(dot(n_center, n_tap)), this). High = geometry edges (corners, silhouettes) are preserved hard while co-planar taps blend freely - which is what we want, since the blotch lives WITHIN a flat surface. Lower = softer normal edge (more cross-edge smoothing / leak); higher = stricter (blotch may survive on gently curved surfaces)
static const float SURFEL_DENOISE_DEPTH_SCALE = 0.05f; // edge-stop tightness on depth: tap weight *= exp(-|z_center - z_tap| / (this * z_center)). RELATIVE to the centre's linear depth so it's distance-invariant (a terrain seen from altitude has huge absolute depth spans; a fixed absolute scale would reject every tap there). Smaller = stricter depth edge (only near-coplanar-in-depth taps blend; preserves depth discontinuities but the blotch may survive at grazing angles); larger = looser (more smoothing, risk of bleeding across depth steps)
#define SURFEL_GRID_CULLING // if defined, surfels will not be added to grid cells that they do not intersect
#define SURFEL_USE_HASHING // if defined, hashing will be used to retrieve surfels, hashing is good because it supports infinite world trivially, but slower due to hash collisions
#define SURFEL_ENABLE_INFINITE_BOUNCES // if defined, previous frame's surfel data will be sampled at ray tracing hit points
#define SURFEL_RAY_SORTING // if defined, the per-frame rays are radix-sorted by their origin surfel's position (Morton order) before tracing, so threads that trace together start from nearby points - far better BVH cache coherence for the otherwise incoherent GI rays. Quality-neutral (identical rays/results, just reordered); adds a sort pass + two uint buffers (key/payload). Comment out to A/B

struct SurfelStats
{
	uint count;
	uint nextCount;
	int deadCount;
	uint cellAllocator;
	uint rayCount;
	int shortage;
	uint spawnCount; // number of surfels spawned this frame (reset each frame)
	uint raySortCount; // count of rays actually allocated this frame (<= SURFEL_RAY_BUDGET); the number of used, contiguous ray slots to radix-sort. Reset each frame like rayCount
};

struct SurfelIndirectArgs
{
	IndirectDispatchArgs iterate;
	IndirectDispatchArgs raytrace;
	IndirectDispatchArgs integrate;
};

#ifdef __cplusplus
static_assert(SURFEL_RECYCLE_RECENCY_MAX < 256, "Must be < 256 because it is packed at 8 bits!");
static_assert(SURFEL_RAY_BOOST_MAX < 256, "Must be < 256 because it is packed at 8 bits!");
#endif // __cplusplus

// This per-surfel surfel structure will be accessed rapidly on GI lookup, so keep it as small as possible
//	But also ensure that it is 16-byte aligned for structured buffer access performance
struct alignas(16) Surfel
{
	SH::L1_RGB::Packed radiance;
	uint2 normal;
	float3 position;
	uint radius_packed; // 16-bit half: per-surfel world radius (distance scaled)

#ifndef __cplusplus
	inline float GetRadius() { return f16tof32(radius_packed); }
	inline void SetRadius(float value) { radius_packed = f32tof16(value); }
#endif // __cplusplus
};
// This per-surfel structure will store all additional persistent data per surfel that isn't needed at GI lookup
struct SurfelData
{
	uint64_t uid;
	uint2 primitiveID;

	uint bary;
	uint raydata; // 24bit rayOffset, 8bit rayCount
	uint properties; // 8bit life frames, 8bit recycle frames, 1bit backface normal (bit16), 1bit seen-this-frame (bit17), 1bit redundant/thin (bit19)
	float max_inconsistency;

	inline uint GetRayOffset() { return raydata & 0xFFFFFF; }
	inline uint GetRayCount() { return (raydata >> 24u) & 0xFF; }

	uint GetLife() { return properties & 0xFF; }
	uint GetRecycle() { return (properties >> 8u) & 0xFF; }
	bool IsBackfaceNormal() { return (properties >> 16u) & 0x1; }
	bool IsSeen() { return (properties >> 17u) & 0x1; }
	bool IsRedundant() { return (properties >> 19u) & 0x1; }

	void SetLife(uint value) { properties |= value & 0xFF; }
	void SetRecycle(uint value) { properties |= (value & 0xFF) << 8u; }
	void SetBackfaceNormal(bool value) { if (value) properties |= 1u << 16u; else properties &= ~(1u << 16u); }
	void SetSeen(bool value) { if (value) properties |= 1u << 17u; else properties &= ~(1u << 17u); }
	void SetRedundant(bool value) { if (value) properties |= 1u << 19u; else properties &= ~(1u << 19u); }
};
struct SurfelVarianceData
{
	float3 mean;
	float3 shortMean;
	float vbbr;
	float3 variance;
	float inconsistency;
};
struct SurfelVarianceDataPacked
{
	uint4 data;

#ifndef __cplusplus
	inline void store(SurfelVarianceData varianceData)
	{
		data.x = PackRGBE(varianceData.mean);
		data.y = PackRGBE(varianceData.shortMean);
		data.z = PackRGBE(varianceData.variance);
		data.w = pack_half2(float2(varianceData.vbbr, varianceData.inconsistency));
	}
	inline SurfelVarianceData load()
	{
		SurfelVarianceData varianceData;
		varianceData.mean = UnpackRGBE(data.x);
		varianceData.shortMean = UnpackRGBE(data.y);
		varianceData.variance = UnpackRGBE(data.z);
		float2 other = unpack_half2(data.w);
		varianceData.vbbr = other.x;
		varianceData.inconsistency = other.y;
		return varianceData;
	}
#endif // __cplusplus
};
struct SurfelRayData
{
	float3 direction;
	float depth;
	float3 radiance;
	uint surfelIndex;
};
struct SurfelRayDataPacked
{
	uint4 data;

#ifndef __cplusplus
	inline void store(SurfelRayData rayData)
	{
		data.xy = pack_half4(float4(rayData.direction, rayData.depth));
		data.z = Pack_R11G11B10_FLOAT(rayData.radiance);
		data.w = rayData.surfelIndex;
	}
	inline SurfelRayData load()
	{
		SurfelRayData rayData;
		float4 unpk = unpack_half4(data.xy);
		rayData.direction = unpk.xyz;
		rayData.depth = unpk.w;
		rayData.radiance = Unpack_R11G11B10_FLOAT(data.z);
		rayData.surfelIndex = data.w;
		return rayData;
	}
#endif // __cplusplus
};
struct SurfelGridCell
{
	uint count;
	uint offset;
};
struct PushConstantsSurfelRaytrace
{
	uint instanceInclusionMask;
};
enum SURFEL_DEBUG
{
	SURFEL_DEBUG_NONE,
	SURFEL_DEBUG_NORMAL,
	SURFEL_DEBUG_COLOR,
	SURFEL_DEBUG_POINT,
	SURFEL_DEBUG_RANDOM,
	SURFEL_DEBUG_HEATMAP,
	SURFEL_DEBUG_INCONSISTENCY,

	SURFEL_DEBUG_FORCE_UINT = 0xFFFFFFFF,
};
struct SurfelDebugPushConstants
{
	SURFEL_DEBUG debug;
};

#ifndef __cplusplus
// World-space cell size (and surfel radius) of cascaded grid level L. Level 0
// is the finest (SURFEL_MIN_RADIUS); each coarser level doubles. A surfel is
// placed at the level whose cell matches its radius, so near surfaces use fine
// surfels and distant ones use a few large surfels without any surfel exceeding
// one cell.
inline float surfel_cellsize(uint level)
{
	return SURFEL_MIN_RADIUS * (float)(1u << level);
}
inline int3 surfel_cell(float3 position, uint level)
{
	const float cell_size = surfel_cellsize(level);
#ifdef SURFEL_USE_HASHING
	return floor(position / cell_size);
#else
	return floor((position - floor(GetCamera().position)) / cell_size) + SURFEL_GRID_DIMENSIONS / 2;
#endif // SURFEL_USE_HASHING
}
// Cascaded grid level a surfel at this position should live at, chosen so it
// has a roughly constant screen-space footprint (SURFEL_RADIUS_PIXELS). The
// finest level is 0 (SURFEL_MIN_RADIUS, sub-unit), so near surfaces keep
// shrinking and getting denser as the camera approaches; each level's cell
// matches its radius so finer levels stay solid (no sub-cell voids).
// projection[1][1] = 1/tan(fovY/2), so 2*dist/(projection[1][1]*height) is the
// world size of one screen pixel here.
// Continuous (unfloored, unclamped) cascade level for a position: log2 of the
// desired radius over the finest radius. surfel_level() floors+clamps this;
// surfel_stable_level() uses the fractional value for hysteresis/dithering.
inline float surfel_level_continuous(float3 position)
{
	const float dist = distance(position, GetCamera().position);
	const float world_per_pixel =
		2.0 * dist /
		(GetCamera().projection[1][1] * (float)GetCamera().internal_resolution.y);
	// Base target: a constant SURFEL_RADIUS_PIXELS screen footprint. The growth
	// term then makes the footprint (and so the world radius/level) increase
	// SUPER-linearly with distance: near surfaces (dist << GROWTH_DISTANCE) keep
	// scale ~1 and their current fine size, while distant ones grow much larger
	// than a constant footprint would give - so a far surface is covered by far
	// fewer, bigger surfels. This shrinks the far working set (fewer to gather,
	// re-trace, and keep live), which is what tanks FPS when a large area comes
	// into view (e.g. flying up over terrain).
	const float growth = 1.0 + dist / SURFEL_RADIUS_GROWTH_DISTANCE;
	const float desired_radius = SURFEL_RADIUS_PIXELS * world_per_pixel * growth;
	return log2(max(desired_radius / SURFEL_MIN_RADIUS, 1.0));
}
inline uint surfel_level(float3 position)
{
	const float level = floor(surfel_level_continuous(position));
	return (uint)clamp(level, 0, SURFEL_GRID_LEVELS - 1);
}
// Recover a surfel's level from its stored radius (== its level's cell size).
inline uint surfel_level_from_radius(float radius)
{
	return firstbithigh(max(1u, (uint)(radius / SURFEL_MIN_RADIUS + 0.5)));
}
// Stable hash of a surfel index to [0,1). Used to give each surfel its own
// level -change threshold so a region doesn't transition all at once.
inline float surfel_hash01(uint x)
{
	x ^= x >> 16; x *= 0x7feb352du;
	x ^= x >> 15; x *= 0x846ca68bu;
	x ^= x >> 16;
	return (float)(x & 0xffffffu) / (float)0x1000000u;
}
// The cascade level a live surfel should hold this frame, with hysteresis and a
// per-surfel dithered threshold. surfel_level() alone snaps purely to camera
// distance, so when the camera moves a flat region (all at ~one distance)
// crosses a level boundary on the SAME frame: every surfel there re-bins,
// grows, goes redundant and mass thin+respawns together, which POPS. Here each
// surfel only steps its level past a threshold offset by its own stable dither,
// so the region's transitions spread across a band of distance and trickle a
// few at a time; a hysteresis dead-band keeps a surfel hovering at a boundary
// from flip-flopping. A newborn (snap) takes the exact distance level
// immediately, as it has no valid current level yet.
//
// @param[in] position - the surfel's world position.
// @param[in] current_radius - the surfel's stored radius (its current level's
//   cell size); ignored when snap is true.
// @param[in] surfel_index - identifies the surfel; seeds its dither.
// @param[in] snap - true for a newborn: take the distance level with no
//   hysteresis (there is no meaningful current level to hold).
inline uint surfel_stable_level(
	float3 position, float current_radius, uint surfel_index, bool snap)
{
	const float lvl_f = surfel_level_continuous(position);
	const uint max_level = SURFEL_GRID_LEVELS - 1u;
	if (snap)
		return (uint)clamp(floor(lvl_f), 0.0, (float)max_level);

	const uint current = min(surfel_level_from_radius(current_radius), max_level);
	// Per-surfel threshold offset in [-0.5, 0.5] * dither, plus a fixed
	// hysteresis half-band: step up/down only well past the boundary, else
	// hold.
	const float dither =
		(surfel_hash01(surfel_index) - 0.5) * SURFEL_LEVEL_DITHER;
	const float hysteresis = 0.25;
	if (lvl_f >= (float)current + 1.0 + hysteresis + dither)
		return min(current + 1u, max_level);
	if (lvl_f < (float)current - hysteresis + dither)
		return (current > 0u) ? (current - 1u) : 0u;
	return current;
}
// Poisson-disk spawn/thinning spacing (as a fraction of radius) for a cascade
// level. Interpolates from the dense near spacing (SURFEL_SPAWN_MIN_SPACING at
// level 0, the closest surfaces) to the sparse far spacing
// (SURFEL_SPAWN_MAX_SPACING at the coarsest level, the most distant surfaces),
// so distant surfels spread out further while the closest level keeps its
// density. Both the spawn spacing gate (surfel_coverage) and the thinning
// threshold (surfel_integrate) use this, so they stay consistent per level.
inline float surfel_spawn_spacing(uint level)
{
	const float t = (float)level / (float)(SURFEL_GRID_LEVELS - 1);
	return lerp(SURFEL_SPAWN_MIN_SPACING, SURFEL_SPAWN_MAX_SPACING, saturate(t));
}
float3 surfel_griduv(float3 position)
{
#ifdef SURFEL_USE_HASHING
	return 0; // hashed grid can't be sampled for colors, it doesn't make sense
#else
	return (((position - floor(GetCamera().position)) / SURFEL_MIN_RADIUS) + SURFEL_GRID_DIMENSIONS / 2) / SURFEL_GRID_DIMENSIONS;
#endif // SURFEL_USE_HASHING
}
// Flat index into the combined (all-levels) grid table. Each level owns a
// SURFEL_TABLE_SIZE-sized block, so a cell never collides across levels.
inline uint surfel_cellindex(int3 cell, uint level)
{
	const uint level_base = level * SURFEL_TABLE_SIZE;
#ifdef SURFEL_USE_HASHING
	const uint p1 = 73856093;   // some large primes
	const uint p2 = 19349663;
	const uint p3 = 83492791;
	int n = p1 * cell.x ^ p2 * cell.y ^ p3 * cell.z;
	n %= SURFEL_TABLE_SIZE;
	return level_base + (uint)n;
#else
	return level_base + flatten3D(cell, SURFEL_GRID_DIMENSIONS);
#endif // SURFEL_USE_HASHING
}
inline bool surfel_cellvalid(int3 cell)
{
#ifdef SURFEL_USE_HASHING
	return true;
#else
	if (cell.x < 0 || cell.x >= SURFEL_GRID_DIMENSIONS.x)
		return false;
	if (cell.y < 0 || cell.y >= SURFEL_GRID_DIMENSIONS.y)
		return false;
	if (cell.z < 0 || cell.z >= SURFEL_GRID_DIMENSIONS.z)
		return false;
	return true;
#endif // SURFEL_USE_HASHING
}
inline bool surfel_cellintersects(Surfel surfel, int3 cell, uint level)
{
	if (!surfel_cellvalid(cell))
		return false;

#ifdef SURFEL_GRID_CULLING
	const float cell_size = surfel_cellsize(level);
#ifdef SURFEL_USE_HASHING
	float3 gridmin = cell * cell_size;
	float3 gridmax = (cell + 1) * cell_size;
#else
	float3 gridmin = cell - SURFEL_GRID_DIMENSIONS / 2 * cell_size + floor(GetCamera().position);
	float3 gridmax = (cell + 1) - SURFEL_GRID_DIMENSIONS / 2 * cell_size + floor(GetCamera().position);
#endif // SURFEL_USE_HASHING

	float3 closestPointInAabb = min(max(surfel.position, gridmin), gridmax);
	float dist = distance(closestPointInAabb, surfel.position);
	if (dist < surfel.GetRadius())
		return true;
	return false;
#else
	return true;
#endif // SURFEL_GRID_CULLING
}
// 27 neighbor offsets in a 3D grid, including center cell:
static const int3 surfel_neighbor_offsets[27] = {
	int3(-1, -1, -1),
	int3(-1, -1, 0),
	int3(-1, -1, 1),
	int3(-1, 0, -1),
	int3(-1, 0, 0),
	int3(-1, 0, 1),
	int3(-1, 1, -1),
	int3(-1, 1, 0),
	int3(-1, 1, 1),
	int3(0, -1, -1),
	int3(0, -1, 0),
	int3(0, -1, 1),
	int3(0, 0, -1),
	int3(0, 0, 0),
	int3(0, 0, 1),
	int3(0, 1, -1),
	int3(0, 1, 0),
	int3(0, 1, 1),
	int3(1, -1, -1),
	int3(1, -1, 0),
	int3(1, -1, 1),
	int3(1, 0, -1),
	int3(1, 0, 0),
	int3(1, 0, 1),
	int3(1, 1, -1),
	int3(1, 1, 0),
	int3(1, 1, 1),
};

float2 surfel_moment_pixel(uint surfel_index, float3 normal, float3 direction)
{
	uint2 moments_pixel = unflatten2D(surfel_index, SQRT_SURFEL_CAPACITY) * SURFEL_MOMENT_RESOLUTION;
	float3 hemi = mul(get_tangentspace(normal), direction);
	hemi = normalize(hemi);
	hemi.z = saturate(hemi.z);
	float2 moments_uv = encode_hemioct(hemi) * 0.5 + 0.5;
	return moments_pixel + clamp(moments_uv * SURFEL_MOMENT_RESOLUTION, 0.5, SURFEL_MOMENT_RESOLUTION - 0.5);
}
float2 surfel_moment_uv(uint surfel_index, float3 normal, float3 direction)
{
	return surfel_moment_pixel(surfel_index, normal, direction) / SURFEL_MOMENT_ATLAS_TEXELS;
}
// Geometric contribution weight of a surfel at a shading point, in [0,1].
// Used by the coverage GI apply ONLY (the visible result), never the feedback
// paths: the raytrace multi-bounce and birth-seed gathers write the surfel
// cache, so they must average over many surfels with a loose weight; sharpening
// them collapses the contributor count and lets bright outliers propagate and
// run away. The apply is output-only, so the sharp edge-aware weight is safe
// there and is what cleans hard seams. Combines three terms:
//   - radial falloff (1 - d^2/r^2)^2: smooth, zero-slope at the surfel edge.
//   - normal sharpness pow(dotN, k): suppresses grazing-angle contributions.
//   - tangent-plane distance: the key anti-leak. A point on a surface that
//     meets the surfel's surface at a hard edge is offset along the surfel's
//     normal, so it is far from the surfel's plane and gets ~0 weight even
//     though it is within the surfel's radius and the normals are not exactly
//     opposed. Coplanar points (same flat surface) are unaffected, so flat
//     walls keep full coverage while 90-degree seams stop bleeding.
// Callers pass to_surface = shading_pos - surfel.position and the precomputed
// dist2/dotN (and have already gated on dist2 < radius^2 and dotN > 0).
inline float surfel_geometry_weight(float3 to_surface, float3 surfel_normal, float radius, float dist2, float dotN)
{
	float weight = saturate(1 - dist2 / sqr(radius));
	weight *= weight;
	weight *= pow(saturate(dotN), SURFEL_NORMAL_WEIGHT_POWER);
	const float plane_dist = abs(dot(to_surface, surfel_normal));
	weight *= saturate(1 - plane_dist / (radius * SURFEL_PLANE_WEIGHT_SCALE));
	return weight;
}
float surfel_moment_weight(float2 moments, float dist)
{
	float mean = moments.x;
	float mean2 = moments.y;
	if (dist > mean)
	{
		// Chebishev weight
		float variance = abs(sqr(mean) - mean2);
		return max(0, pow(variance / (variance + sqr(max(0, dist - mean))), 3));
	}
	return 1;
}

void MultiscaleMeanEstimator(
	float3 y,
	inout SurfelVarianceData data,
	float shortWindowBlend = 0.08f
)
{
	float3 mean = data.mean;
	float3 shortMean = data.shortMean;
	float vbbr = data.vbbr;
	float3 variance = data.variance;
	float inconsistency = data.inconsistency;

	// Suppress fireflies.
	{
		float3 dev = sqrt(max(1e-5, variance));
		float3 highThreshold = 0.1 + shortMean + dev * 8;
		float3 overflow = max(0, y - highThreshold);
		y -= overflow;
	}

	float3 delta = y - shortMean;
	shortMean = lerp(shortMean, y, shortWindowBlend);
	float3 delta2 = y - shortMean;

	// This should be a longer window than shortWindowBlend to avoid bias
	// from the variance getting smaller when the short-term mean does.
	float varianceBlend = shortWindowBlend * 0.5;
	variance = lerp(variance, delta * delta2, varianceBlend);
	float3 dev = sqrt(max(1e-5, variance));

	float3 shortDiff = mean - shortMean;

	float relativeDiff = dot(float3(0.299, 0.587, 0.114),
		abs(shortDiff) / max(1e-5, dev));
	inconsistency = lerp(inconsistency, relativeDiff, 0.08);

	float varianceBasedBlendReduction =
		clamp(dot(float3(0.299, 0.587, 0.114),
			0.5 * shortMean / max(1e-5, dev)), 1.0 / 32, 1);

	float3 catchUpBlend = clamp(smoothstep(0, 1,
		relativeDiff * max(0.02, inconsistency - 0.2)), 1.0 / 256, 1);
	catchUpBlend *= vbbr;

	vbbr = lerp(vbbr, varianceBasedBlendReduction, 0.1);
	mean = lerp(mean, y, saturate(catchUpBlend));

	// Output
	data.mean = mean;
	data.shortMean = shortMean;
	data.vbbr = vbbr;
	data.variance = variance;
	data.inconsistency = inconsistency;
}

#endif // __cplusplus

#endif // WI_SHADERINTEROP_SURFEL_GI_H
