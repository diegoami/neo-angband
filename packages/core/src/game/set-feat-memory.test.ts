import { describe, expect, it } from "vitest";
import { FEAT, SQUARE } from "../generated/index.js";
import { loc } from "../loc.js";
import { installCaveCommands } from "./cave-cmd.js";
import type { PlayerCommand } from "./context.js";
import { makeState } from "./harness.js";
import { knownFeat, squareMemorize } from "./known.js";
import { createDefaultRegistry, processPlayer } from "./player-turn.js";
import { installChunkFeatHook } from "./trap.js";

/* cave-square.c:1256-1268 calls square_note_spot only during live play.
 * cave-map.c:226-245 permits memory updates only for seen or player grids.
 * These checks intentionally have no post-command updateView/noteSpots pass.
 */
describe("live setFeat memory (engine #4 / upstream #429)", () => {
  it.each([FEAT.OPEN, FEAT.GRANITE])("immediately remembers seen terrain %i", (feat) => {
    const state = makeState({ playerGrid: loc(5, 5), seed: 1 });
    const grid = loc(6, 5);
    squareMemorize(state, grid);
    state.chunk.sqinfoOn(grid, SQUARE.SEEN);
    installChunkFeatHook(state);

    state.chunk.setFeat(grid, feat);

    expect(state.chunk.feat(grid)).toBe(feat);
    expect(knownFeat(state, grid)).toBe(feat);
  });

  it("remembers the player's own square even when it is not seen", () => {
    const state = makeState({ playerGrid: loc(5, 5), seed: 1 });
    const grid = state.actor.grid;
    squareMemorize(state, grid);
    state.chunk.sqinfoOff(grid, SQUARE.SEEN);
    installChunkFeatHook(state);

    state.chunk.setFeat(grid, FEAT.OPEN);

    expect(knownFeat(state, grid)).toBe(FEAT.OPEN);
  });

  it.each([false, true])("does not reveal an unseen remote change (previously known: %s)", (known) => {
    const state = makeState({ playerGrid: loc(5, 5), seed: 1 });
    const grid = loc(10, 10);
    if (known) squareMemorize(state, grid);
    const before = knownFeat(state, grid);
    state.chunk.sqinfoOff(grid, SQUARE.SEEN);
    installChunkFeatHook(state);

    state.chunk.setFeat(grid, FEAT.GRANITE);

    expect(state.chunk.feat(grid)).toBe(FEAT.GRANITE);
    expect(knownFeat(state, grid)).toBe(before);
  });

  it("generation without a live hook clears wall flags without memorizing", () => {
    const state = makeState({ playerGrid: loc(5, 5), seed: 1 });
    const grid = loc(6, 5);
    squareMemorize(state, grid);
    const before = knownFeat(state, grid);
    state.chunk.sqinfoOn(grid, SQUARE.SEEN);
    const flags = [SQUARE.WALL_INNER, SQUARE.WALL_OUTER, SQUARE.WALL_SOLID];
    for (const flag of flags) state.chunk.sqinfoOn(grid, flag);
    expect(state.chunk.onFeatSet).toBeUndefined();

    state.chunk.setFeat(grid, FEAT.GRANITE);

    expect(state.chunk.feat(grid)).toBe(FEAT.GRANITE);
    expect(knownFeat(state, grid)).toBe(before);
    for (const flag of flags) expect(state.chunk.sqinfoHas(grid, flag)).toBe(false);
  });

  it("legal close/open commands keep remembered doors current before the next command", () => {
    /* cmd-cave.c:174-237 and 349-400: unlocked open/close spend a turn;
     * a repeated open on an already open door refuses without spending one.
     */
    const state = makeState({ playerGrid: loc(5, 5), seed: 1 });
    const grid = loc(6, 5);
    state.chunk.setFeat(grid, FEAT.OPEN);
    squareMemorize(state, grid);
    state.chunk.sqinfoOn(grid, SQUARE.SEEN);
    installChunkFeatHook(state);
    const registry = createDefaultRegistry();
    installCaveCommands(registry, {});
    const run = (code: string): number => {
      const commands: PlayerCommand[] = [{ code, dir: 6 }];
      state.nextCommand = (): PlayerCommand | null => commands.shift() ?? null;
      return processPlayer(state, registry).energyUsed;
    };

    for (const [code, feat] of [["close", FEAT.CLOSED], ["open", FEAT.OPEN], ["close", FEAT.CLOSED], ["open", FEAT.OPEN]] as const) {
      expect(run(code)).toBe(state.z.moveEnergy);
      expect(state.actor.grid).toEqual(loc(5, 5));
      expect(state.chunk.feat(grid)).toBe(feat);
      expect(knownFeat(state, grid)).toBe(feat);
    }
    expect(run("open")).toBe(0);
    expect(knownFeat(state, grid)).toBe(FEAT.OPEN);
  });
});
