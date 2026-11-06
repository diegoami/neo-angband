#!/usr/bin/env python3
"""Turn recorded native CAST/OUTCOME lines into a readable offline comparison."""
import json
from pathlib import Path
import re
import sys

CASES = {
    "speed-wins": ("Haste is chosen", "Haste Self", "The Borg chooses haste, which makes the character faster. The original code also casts regeneration first. The correction casts only haste."),
    "regen-only": ("Regeneration is chosen", "Rapid Regeneration", "The Borg chooses regeneration, which makes health recover faster. The original code casts it twice. The correction casts it once."),
    "speed-only": ("Control: only haste is learned", "Haste Self", "Regeneration is not learned. Both versions cast haste once, spending 12 mana."),
    "already-active": ("Control: regeneration is already active", "No spell", "The regeneration effect is already active. Both versions leave it alone and spend no mana."),
    "unlearned": ("Control: regeneration is not learned", "No spell", "The character has not learned regeneration. Neither version casts it."),
    "blind": ("Control: character is blind", "No spell", "Blindness prevents this preparation. Neither version casts a spell."),
    "confused": ("Control: character is confused", "No spell", "Confusion prevents this preparation. Neither version casts a spell."),
    "town": ("Control: character is in town", "No spell", "This preparation is not used in town. Neither version casts a spell."),
    "low-mana": ("Control: not enough spare mana", "No spell", "The character has too little spare mana for preparation. Neither version casts a spell."),
}

def parse(path):
    raw = path.read_text()
    setup = re.search(r"SETUP .*? hp=(\d+)/(\d+) sp=(\d+)/(\d+)", raw)
    outcome = re.search(r"OUTCOME (.*)", raw)
    if not setup or not outcome:
        raise RuntimeError(f"Incomplete native recording: {path}")
    casts = [{"spell": spell, "before": int(before), "after": int(after),
              "energy": int(energy), "regen": int(heal), "haste": int(fast)}
             for spell, before, after, energy, heal, fast in re.findall(
                 r"CAST spell=(.*?) sp_before=(\d+) sp_after=(\d+) energy=(\d+) heal=(\d+) fast=(\d+)", raw)]
    result = {key: int(value) for key, value in re.findall(r"(\w+)=(\d+)", outcome[1])}
    if len(casts) != result["commands"]:
        raise RuntimeError(f"CAST count disagrees with native outcome: {path}")
    hp, max_hp, mana, max_mana = map(int, setup.groups())
    if sum(c["before"] - c["after"] for c in casts) != result["mana_spent"]:
        raise RuntimeError(f"Mana accounting disagrees: {path}")
    return {"hp": hp, "max_hp": max_hp, "start": mana, "max": max_mana,
            "casts": casts, "outcome": result, "raw": raw}

def generate(directory):
    data = {}
    for mode, (label, chosen, explanation) in CASES.items():
        data[mode] = {"label": label, "chosen": chosen, "explanation": explanation,
                      "before": parse(directory / f"before-{mode}.log"),
                      "after": parse(directory / f"after-{mode}.log")}
    payload = json.dumps(data, ensure_ascii=False).replace("<", "\\u003c")
    html = TEMPLATE.replace("__RECORDED_DATA__", payload)
    (directory / "replay.html").write_text(html)
    (directory / "replay-data.json").write_text(json.dumps(data, indent=2) + "\n")
    print(f"Open in your browser: {directory / 'replay.html'}")

TEMPLATE = r'''<!doctype html>
<html lang="en"><meta charset="utf-8"><meta name="viewport" content="width=device-width,initial-scale=1">
<title>Why the Borg casts an extra spell</title>
<style>
*{box-sizing:border-box}body{margin:0;background:#f4f2ec;color:#202b33;font:17px/1.6 system-ui,sans-serif}main{max-width:1080px;margin:auto;padding:36px 24px 60px}.eyebrow{font-size:13px;font-weight:700;letter-spacing:.08em;text-transform:uppercase;color:#516367}h1{font-size:clamp(30px,5vw,48px);line-height:1.15;max-width:900px;margin:12px 0 22px}h2{font-size:23px;margin:0 0 14px}p{max-width:900px}.lede{font-size:20px}.controls{padding:22px;background:white;border-radius:14px;border:1px solid #d8ded9;margin-top:28px}select,button{font:inherit;padding:10px 15px;border:1px solid #a8b8b5;border-radius:8px;background:white;color:inherit}select{max-width:100%;margin:8px 0 18px}button{cursor:pointer}button.active{background:#235f59;color:white;border-color:#235f59}.buttons{display:flex;gap:10px;flex-wrap:wrap}.panels{display:grid;grid-template-columns:1fr 1fr;gap:20px;margin:22px 0}.panel{border-radius:14px;border:1px solid #d4d9d3;padding:24px;background:white}.panel.original{border-top:6px solid #b4773e}.panel.corrected{border-top:6px solid #267568}.chosen{font-weight:650}.mana{font-size:42px;line-height:1.2;font-weight:750;margin:18px 0 8px}.mana small{font-size:18px;font-weight:400}.bar{height:12px;background:#e7eeea;border-radius:7px;overflow:hidden}.fill{height:100%;background:#4c777d;transition:width .15s}.stats{font-size:15px;color:#4a5a60}.action{min-height:90px;margin-top:20px;padding:15px;background:#f1f5f2;border-radius:8px}.sequence{min-height:80px;padding-left:24px}li{margin:6px 0}.muted{color:#65716f}.outcome{background:#e4eeea;border-left:5px solid #267568;padding:18px 22px;border-radius:6px}.limit{font-size:15px;max-width:920px}details{background:white;padding:16px 20px;border-radius:8px;margin:18px 0}summary{cursor:pointer;font-weight:650}pre{white-space:pre-wrap;overflow-wrap:anywhere;font:13px/1.5 ui-monospace,monospace;max-height:360px;overflow:auto}a{color:#205d61}@media(max-width:650px){.panels{grid-template-columns:1fr}main{padding:25px 16px}.action,.sequence{min-height:0}}
</style><main>
<div class="eyebrow">Recorded native C game test · two PRs, one bug</div>
<h1>The Borg casts a spell while deciding what to cast</h1>
<p class="lede">It should compare its options, choose one, then cast it. The original code adds a regeneration cast while comparing, and then adds the chosen spell too.</p>
<p><strong>Rapid Regeneration</strong> makes health recover faster. <strong>Haste Self</strong> makes the character faster. This replay follows the actual spells and mana changes recorded by the native game test.</p>
<section class="controls"><label for="scenario"><strong>Choose a recorded situation</strong></label><br><select id="scenario"></select><p id="explanation"></p><div class="chosen">Chosen preparation: <span id="chosen"></span></div><p>Click through the same decision in both versions:</p><div class="buttons"><button data-step="0">Start</button><button data-step="1">Show first cast</button><button data-step="2" class="active">Show final result</button></div></section>
<div class="panels" aria-live="polite"><section class="panel original" id="before"><h2>Original Borg</h2><div class="mana"></div><div class="bar"><div class="fill"></div></div><p class="stats"></p><div class="action"></div><ol class="sequence"></ol></section><section class="panel corrected" id="after"><h2>Corrected Borg</h2><div class="mana"></div><div class="bar"><div class="fill"></div></div><p class="stats"></p><div class="action"></div><ol class="sequence"></ol></section></div>
<p class="outcome" id="outcome"></p>
<h2>What the comparison establishes</h2><p>The two affected situations have one extra cast in the original version, costing <strong>20 mana</strong>. The correction casts only the selected spell. The other seven situations keep the same behavior.</p>
<p class="limit"><strong>Important limit:</strong> a second regeneration cast extends its duration. This test does not show that the extra cast is always harmful, nor does it demonstrate a death or a worse fight. It establishes that the Borg casts an extra spell it did not select and pays for it.</p>
<details><summary>How these results were captured</summary><p>A synthetic test gives a Druid the relevant book, learned spells, full health and enough mana. The real Borg preparation code chooses and requests the spells. The test decodes those requests and executes the corresponding native game commands. The game computes the mana cost and spell effects. No monsters or world turns run between these commands, so casts are counted here; extra enemy attacks are not.</p><p>This is a replay of recorded command results, not a live game or an invented combat animation. Health is displayed from the native setup; the fixture has no damaging attack. Regeneration duration increases in the duplicate-cast case.</p></details>
<details><summary>Read the recorded native output for this situation</summary><h3>Original</h3><pre id="before-log"></pre><h3>Corrected</h3><pre id="after-log"></pre><p>In the logs, <code>sp_before</code> and <code>sp_after</code> mean mana before and after each native cast.</p></details>
<p class="limit"><a href="https://github.com/diegoami/neo-angband-tracker/pull/264">#264 changes the game code</a>. <a href="https://github.com/diegoami/neo-angband-tracker/pull/265">#265 contains the tests and recorded evidence</a>. Discovery credit: borg-rs.</p>
</main><script>
const data=__RECORDED_DATA__;
const select=document.getElementById('scenario');
for(const [key,c] of Object.entries(data)){const option=document.createElement('option');option.value=key;option.textContent=c.label;select.append(option)}
let step=2;
function draw(){
 const c=data[select.value];document.getElementById('explanation').textContent=c.explanation;document.getElementById('chosen').textContent=c.chosen;
 for(const phase of ['before','after']){
  const d=c[phase],panel=document.getElementById(phase),shown=d.casts.slice(0,step),last=shown.at(-1),mana=last?last.after:d.start;
  panel.querySelector('.mana').innerHTML=`${mana} <small>/ ${d.max} mana</small>`;
  panel.querySelector('.fill').style.width=(100*mana/d.max)+'%';
  panel.querySelector('.stats').textContent=`Health: ${d.hp}/${d.max_hp}. Mana spent so far: ${d.start-mana}. Casts performed: ${shown.length}.`;
  panel.querySelector('.action').textContent=step===0?'No spell has been cast yet.':last?`Last native action: cast ${last.spell}. Mana ${last.before} → ${last.after}.`: 'No spell is cast in this situation.';
  const list=panel.querySelector('.sequence');list.replaceChildren();
  for(const cast of shown){const li=document.createElement('li');li.textContent=`${cast.spell}: ${cast.before-cast.after} mana`;list.append(li)}
  if(step===2){const li=document.createElement('li');li.className='muted';li.textContent='Decision complete.';list.append(li)}
  document.getElementById(phase+'-log').textContent=d.raw;
 }
 const old=c.before.outcome,newer=c.after.outcome,difference=old.mana_spent-newer.mana_spent;
 document.getElementById('outcome').textContent=difference?`Recorded final difference: ${old.commands} casts → ${newer.commands} cast; ${difference} less mana spent. Both versions start at ${c.before.start} mana.`:'Unchanged control: both versions perform the same casts and spend the same mana.';
 for(const b of document.querySelectorAll('[data-step]'))b.classList.toggle('active',Number(b.dataset.step)===step);
}
select.addEventListener('change',()=>{step=2;draw()});
for(const b of document.querySelectorAll('[data-step]'))b.addEventListener('click',()=>{step=Number(b.dataset.step);draw()});
draw();
</script></html>'''

if __name__ == "__main__":
    generate(Path(sys.argv[1]).resolve())
