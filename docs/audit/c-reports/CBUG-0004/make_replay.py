#!/usr/bin/env python3
"""Display recorded native decisions and attack outcomes, without recomputing damage."""
import json
from pathlib import Path
import re
import sys

CASES = {
 "acid-only": ("Acid protection only: cold bite underestimated", "The Rogue has protection from acid, but not cold. The original Borg stays beside the snow troll. The correction casts Phase Door and moves away before the scheduled attack."),
 "cold-only": ("Cold protection only: show the tradeoff", "The Rogue really has cold protection. The original teleports away for 2 mana. The correction stays and spends no mana, but takes 20 HP from the scheduled attack. This is a tradeoff, not an HP improvement."),
 "neither": ("Control: neither protection", "Both versions recognize the cold threat and cast Phase Door."),
 "both": ("Control: both protections", "Both versions stay beside the troll. Cold protection does not remove the attack's physical damage."),
 "physical-acid-control": ("Control: scorpion, acid protection", "The scorpion has no cold blow. Both versions escape using the same Phase Door scroll."),
 "physical-cold-control": ("Control: scorpion, cold protection", "Changing cold protection does not change this scorpion evaluation. Both versions use the same scroll."),
}

def generate(directory):
 data = {}
 for mode, (label, explanation) in CASES.items():
  case = {"label": label, "explanation": explanation}
  for phase in ("before", "after"):
   text = (directory / f"{phase}-{mode}.log").read_text()
   frames = [json.loads(line[6:]) for line in text.splitlines() if line.startswith("FRAME ")]
   outcome = re.search(r"OUTCOME (.*)", text)
   danger = re.search(r"DANGER current=(\d+) escape_threshold=(\d+)", text)
   if len(frames) != 3 or not outcome or not danger: raise RuntimeError(f"Incomplete capture: {phase}-{mode}")
   result = {k: int(v) for k, v in re.findall(r"(\w+)=(\d+)", outcome[1])}
   if frames[0]["hp"]-frames[2]["hp"] != result["hp_lost"]: raise RuntimeError("Native frame/outcome HP mismatch")
   case[phase] = {"frames": frames, "result": result, "danger": int(danger[1]), "threshold": int(danger[2]), "raw": text}
  data[mode] = case
 payload = json.dumps(data).replace("<", "\\u003c")
 (directory / "replay.html").write_text(TEMPLATE.replace("__DATA__", payload))
 (directory / "replay-data.json").write_text(json.dumps(data, indent=2)+"\n")
 print(f"Open in your browser: {directory / 'replay.html'}")

TEMPLATE = r'''<!doctype html><html lang="en"><meta charset="utf-8"><meta name="viewport" content="width=device-width,initial-scale=1"><title>Acid protection is not cold protection</title>
<style>*{box-sizing:border-box}body{margin:0;background:#f4f2ec;color:#223139;font:17px/1.6 system-ui,sans-serif}main{max-width:1080px;margin:auto;padding:35px 24px 60px}h1{font-size:clamp(30px,5vw,48px);line-height:1.15;max-width:930px}h2{font-size:23px}.intro{font-size:20px}.card{padding:23px;background:white;border:1px solid #d1dad4;border-radius:12px;margin:20px 0}.panels{display:grid;grid-template-columns:1fr 1fr;gap:20px}.panels .card:first-child{border-top:6px solid #aa783e}.panels .card:last-child{border-top:6px solid #377d71}.hp{font-size:40px;font-weight:750}.hp small{font-size:18px;font-weight:400}.bar{height:12px;background:#e7eeea;border-radius:6px;overflow:hidden}.fill{height:100%;background:#538678}.stats{font-size:16px}.action{padding:15px;background:#f0f4f1;border-radius:7px;min-height:100px}select,button{font:inherit;padding:10px 14px;border:1px solid #a8b7b2;background:white;border-radius:7px;max-width:100%}button{cursor:pointer}button.active{background:#245f59;color:white}.buttons{display:flex;flex-wrap:wrap;gap:10px;margin-top:18px}.summary{border-left:5px solid #34776c;background:#e4eeea;padding:18px 22px}.limit{font-size:15px}details{margin:18px 0;background:white;padding:16px 20px;border-radius:8px}summary{cursor:pointer;font-weight:650}pre{white-space:pre-wrap;overflow-wrap:anywhere;max-height:360px;overflow:auto;font:13px/1.5 ui-monospace,monospace}a{color:#24616a}@media(max-width:650px){.panels{grid-template-columns:1fr}main{padding:24px 16px}}</style>
<main><p>Recorded native C choice and attack · no invented damage</p><h1>Acid protection should not count as cold protection</h1><p class="intro">A Rogue protected from acid stands beside a snow troll with cold bites. The original Borg stays and loses 21 HP in the scheduled attack. The correction recognizes that cold protection is missing, casts Phase Door and avoids that attack, spending 2 mana.</p><p><strong>The reverse case matters too:</strong> with cold protection alone, the correction stays and saves 2 mana but takes 20 HP. The change is not uniformly better for HP. Both results are shown below.</p>
<section class="card"><label for="scenario"><strong>Choose a recorded situation</strong></label><br><select id="scenario"></select><p id="explanation"></p><div class="buttons"><button data-step="0">Start</button><button data-step="1">Show decision</button><button data-step="2" class="active">Show attack</button></div></section>
<div class="panels" aria-live="polite"><section id="before" class="card"><h2>Original Borg</h2><div class="hp"></div><div class="bar"><div class="fill"></div></div><p class="stats"></p><p class="action"></p></section><section id="after" class="card"><h2>Corrected resistance check</h2><div class="hp"></div><div class="bar"><div class="fill"></div></div><p class="stats"></p><p class="action"></p></section></div><p id="outcome" class="summary"></p>
<h2>What the test measures</h2><p>The real Borg chooses whether to escape. The test executes its actual spell or scroll command through the native game. It then schedules one normal monster attack <strong>only if the monster remains adjacent</strong>. The real game determines hits and damage.</p><p class="limit">This isolates the escape decision. It does not run the rest of the Borg's turn, monster pursuit, other attacks or a complete fight. It does not establish how often this situation occurs, that the Borg would choose the same later action in full play, a death, or an overall survival improvement. Both versions use unchanged setup, seed and assertions.</p><p class="limit">Phase Door is a short teleport. Casting it can fail; the Rogue's native failure chance here is 5%. The successful escape shown is the actual fixed-seed result, not a guaranteed outcome for every game.</p>
<details><summary>Why the correction changes these decisions</summary><p>The Borg mistakenly applies acid protection when estimating a cold melee attack. The correction applies cold protection instead. In the acid-only recording the estimated danger changes from 52 to116, crossing the existing escape threshold104. With cold protection alone it changes from116 to52. These are decision scores, not measured HP damage. The HP values above come from the separate native attack.</p><p>Cold protection does not eliminate the physical part of a bite or the troll's claws. That is why the protected character can still lose20 HP after choosing to stay. No damage rule or escape threshold was changed.</p></details>
<details><summary>Recorded game output for this situation</summary><h3>Original</h3><pre id="before-log"></pre><h3>Corrected</h3><pre id="after-log"></pre></details><p class="limit">Canonical report: <a href="https://github.com/diegoami/neo-angband-tracker/issues/26">issue26 / CBUG-0004</a>. Discovery credit: borg-rs. Original source and full reproduction details accompany this capture.</p></main>
<script>const data=__DATA__;const select=document.getElementById('scenario');let step=2;for(const [key,c] of Object.entries(data)){const o=document.createElement('option');o.value=key;o.textContent=c.label;select.append(o)}
function draw(){const c=data[select.value];document.getElementById('explanation').textContent=c.explanation;for(const phase of ['before','after']){const d=c[phase],f=d.frames[step],p=document.getElementById(phase);p.querySelector('.hp').innerHTML=`${f.hp} <small>/ ${f.max_hp} HP</small>`;p.querySelector('.fill').style.width=(100*f.hp/f.max_hp)+'%';p.querySelector('.stats').textContent=`Mana: ${f.mana}/${f.max_mana}. Phase Door scrolls: ${f.scrolls}. Distance from monster: ${f.distance}.`;let action=step===0?'Starts beside the monster, before the escape check.':f.distance>1?(d.result.scrolls_spent?'Reads a Phase Door scroll and moves away.':'Casts Phase Door and moves away.'):'Stays beside the monster.';if(step===2)action+=d.result.melee?` The scheduled attack costs ${d.result.hp_lost} HP.`:' The monster is no longer adjacent, so no melee attack is made.';p.querySelector('.action').textContent=action;document.getElementById(phase+'-log').textContent=d.raw}const b=c.before.result,a=c.after.result;document.getElementById('outcome').textContent=select.value==='cold-only'?`Tradeoff: original loses ${b.hp_lost} HP and spends ${b.mana_spent} mana; correction loses ${a.hp_lost} HP and spends ${a.mana_spent} mana.`:select.value==='acid-only'?`Recorded attack: ${b.hp_lost} HP lost → ${a.hp_lost} HP lost. Escape costs ${a.mana_spent} mana.`:'Unchanged control: actions, HP and resources match before and after.';for(const b of document.querySelectorAll('[data-step]'))b.classList.toggle('active',Number(b.dataset.step)===step)}
select.addEventListener('change',()=>{step=2;draw()});for(const b of document.querySelectorAll('[data-step]'))b.addEventListener('click',()=>{step=Number(b.dataset.step);draw()});draw();</script></html>'''

if __name__ == "__main__": generate(Path(sys.argv[1]).resolve())
