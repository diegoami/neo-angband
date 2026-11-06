"""Render a self-contained, offline replay of captured native C frames."""
import json
from pathlib import Path
import sys

root=Path(sys.argv[1]).resolve() if len(sys.argv)>1 else Path(__file__).resolve().parent
before=json.loads((root/'before-expanded.json').read_text())['cases']
after=json.loads((root/'after-expanded.json').read_text())['cases']
labels={
 'both-helpful': ('Two threats; both backups help', 'A white dragon and a red dragon threaten the Borg. The spare shield protects against both frost and fire; the spare dagger protects only against frost. Both swaps are allowed by the existing safety threshold. Watch which item the Borg selects, then compare the damage from one breath from each dragon.'),
 'fire-armour': ('Fire: useful shield overlooked', 'A red dragon has already breathed twice. The spare fire-resistant shield helps; the frost dagger does not help against fire. Watch the equipment decision and the next fire breath.'),
 'electric-weapon': ('Electricity: useful weapon overlooked', 'A blue dragon has already breathed twice. The lightning dagger protects against electricity; the cold-resistant shield does not. Watch the equipment decision and the next lightning breath.'),
 'armour-better': ('Frost: useful shield overlooked', 'The original case: after two preparation frost breaths, a cold-resistant shield is in the pack alongside a fire dagger. The test asks the native emergency-swap routine what to do, applies its selected item, then schedules one more native breath.'),
 'weapon-better': ('Frost: useful weapon overlooked', 'The mirrored original case: the dagger resists frost and the spare shield resists fire. The same native decision and frost attack show whether the weapon is chosen.'),
 'armour-only': ('Control: only the useful shield', 'There is no backup weapon. Both versions should equip the cold-resistant shield and produce identical results.'),
 'weapon-only': ('Control: only the useful weapon', 'There is no backup armour. Both versions should wield the frost dagger and produce identical results.'),
 'tie': ('Control: equal protection', 'Both spare items resist cold and have equal danger scores. Both versions should keep the existing weapon preference on a tie.'),
 'no-benefit': ('Control: neither item helps', 'Both spares resist fire while the attacker breathes frost. Both versions should leave the current equipment alone.'),
}
rows={m:{'title':labels[m][0],'description':labels[m][1],'before':before[m],'after':after[m]} for m in labels}
for mode,row in rows.items():
 assert [f['stage'] for f in row['before']['frames']]==[f['stage'] for f in row['after']['frames']],mode
payload=json.dumps(rows).replace('<','\\u003c')
page=r'''<!doctype html>
<html lang="en"><meta charset="utf-8"><meta name="viewport" content="width=device-width, initial-scale=1">
<title>Equipment-swap comparison — native C replay</title>
<style>
:root{font-family:system-ui,sans-serif;color:#172538;background:#edf2f6}body{max-width:1180px;margin:28px auto;padding:0 20px}h1{font-size:27px;margin:0 0 8px}p{line-height:1.5}.muted{color:#526276;font-size:14px}.toolbar,.summary,.panel{background:white;border:1px solid #d7dfe8;border-radius:12px;padding:16px}.toolbar{display:flex;gap:12px;align-items:center;flex-wrap:wrap}select{max-width:100%;font-size:16px;padding:8px}button{font:inherit;background:#1a4d75;color:white;border:0;border-radius:6px;padding:9px 13px;cursor:pointer}button.secondary{background:#e4ebf2;color:#20364d}button:focus,select:focus,input:focus{outline:3px solid #7ac3ec;outline-offset:2px}input{flex:1;min-width:160px}.columns{display:grid;grid-template-columns:1fr 1fr;gap:16px;margin-top:16px}.panel h2{margin:0 0 12px;font-size:22px}.metrics{display:flex;gap:18px;flex-wrap:wrap}.hp{font-size:25px;font-weight:700}.bar{height:9px;background:#dce4ed;border-radius:5px;overflow:hidden;margin:6px 0 14px}.bar span{display:block;height:100%;background:#b35445}.fixed .bar span{background:#23745f}pre.map{font:16px/1.1 ui-monospace,Consolas,monospace;background:#101d2a;color:#eef5fb;padding:12px;overflow:auto;border-radius:8px;max-width:100%;width:max-content;margin:12px auto}.gear{font-size:14px;margin:9px 0;overflow-wrap:anywhere}.gear strong{display:block;color:#41566d}.message{min-height:46px;background:#f0f4f8;padding:10px;border-radius:6px;white-space:pre-wrap;font-size:14px}.summary{margin:16px 0}.result{font-weight:700;font-size:20px}.risk{font-size:14px;padding-top:8px}details{margin-top:16px}pre.raw{white-space:pre-wrap;font:12px/1.4 ui-monospace,monospace;max-height:400px;overflow:auto;background:white;padding:14px}#stage{font-size:21px;margin:18px 0 5px}a{color:#1a4d75}@media(max-width:760px){.columns{grid-template-columns:1fr}body{margin:18px auto;padding:0 12px}pre.map{font-size:14px}h1{font-size:23px}}
</style>
<h1>Which backup item does the Borg choose?</h1>
<p class="muted">Captured from the native C engine and Borg. This is a replay of a controlled test, not a live autonomous game. The map uses the native ASCII renderer; HP, items, messages and decisions are recorded run data.</p>
<label for="case"><strong>Scenario</strong></label><br><select id="case"></select>
<p id="description"></p>
<div class="summary"><div id="result" class="result"></div><div id="backups" class="muted"></div></div>
<div class="toolbar"><button id="previous" class="secondary">Previous</button><button id="play">Play</button><button id="next" class="secondary">Next</button><input id="step" type="range" min="0" value="0" aria-label="Replay step"><span id="counter"></span><button id="decision" class="secondary">Show decision</button><button id="damage" class="secondary">Show damage</button></div>
<h2 id="stage"></h2><p id="explanation" class="muted"></p>
<div class="columns"><section id="before" class="panel"></section><section id="after" class="panel fixed"></section></div>
<p class="muted">Map: @ = Borg; d = dragon; . = visible floor. Blank areas are not rendered as known terrain. The two versions use the same scenario, seed and scheduled attacks; only the emergency item comparison differs.</p>
<details><summary>Read the native output and setup limits</summary><p>The test constructs an identified inventory, level/depth and illuminated arena, schedules the listed dragon breaths, calls the production emergency-swap routine, decodes its actual queued inventory letter, and applies that choice with native inven_wield. Damage and resistances come from native monster spells. Other healing/escape choices and full UI turn timing are outside this isolated test. Danger scores are the Borg's estimates, not measured damage. The four control cases must be identical. No player-death or complete-run frequency claim is made.</p><div class="columns"><pre id="raw-before" class="raw"></pre><pre id="raw-after" class="raw"></pre></div></details>
<p><button id="download" class="secondary">Download captured data</button></p>
<script>
const cases=__DATA__;
const $=id=>document.getElementById(id);let timer=null,mode='both-helpful',step=0;
for(const [id,c] of Object.entries(cases)){const o=document.createElement('option');o.value=id;o.textContent=c.title;$('case').append(o)}
function stop(){if(timer)clearInterval(timer);timer=null;$('play').textContent='Play'}
function text(parent,tag,value,cls){const n=document.createElement(tag);n.textContent=value;if(cls)n.className=cls;parent.append(n);return n}
function field(parent,label,value){const n=document.createElement('div');n.className='gear';text(n,'strong',label);text(n,'span',value);parent.append(n)}
function draw(which,title){const c=cases[mode][which],f=c.frames[step],p=$(which);p.replaceChildren();text(p,'h2',title);text(p,'div',`HP ${f.hp} / ${f.max_hp}`,'hp');const bar=document.createElement('div');bar.className='bar';const fill=document.createElement('span');fill.style.width=(100*Math.max(0,f.hp)/f.max_hp)+'%';bar.append(fill);p.append(bar);text(p,'div',`Resistance — cold: ${f.cold}, fire: ${f.fire}, electricity: ${f.elec}`,'muted');field(p,'Equipped shield',f.equipped_shield);field(p,'Equipped weapon',f.equipped_weapon);if(f.armour_danger>=0||f.weapon_danger>=0)text(p,'div',`Pre-swap danger: current ${f.danger}; armour ${f.armour_danger<0?'absent':f.armour_danger}; weapon ${f.weapon_danger<0?'absent':f.weapon_danger}`,'risk');text(p,'pre',f.map.join('\n'),'map');text(p,'div',f.message||'No game message at this step.','message');}
function render(){const c=cases[mode],f=c.before.frames[step];$('description').textContent=c.description;$('step').max=c.before.frames.length-1;$('step').value=step;$('counter').textContent=`${step+1} / ${c.before.frames.length}`;$('stage').textContent=f.stage;let note='';if(f.stage.includes('preparation'))note='Controlled setup: the real dragon breath damages the character before the equipment decision.';if(f.stage.includes('compares'))note='The native Borg calculates danger for each real backup item. Lower is safer; the existing acceptance threshold still applies.';if(f.stage.includes('decision'))note='The observer applies the item actually selected by the Borg. Compare the equipped items and elemental resistances.';if(f.stage.includes('hits'))note='The native engine resolves the scheduled breath attack. Compare remaining HP.';$('explanation').textContent=note;const saved=c.before.hp_lost-c.after.hp_lost;$('result').textContent=`Final test damage: unfixed ${c.before.hp_lost} HP · corrected ${c.after.hp_lost} HP · ${saved} HP difference`;$('backups').textContent=`Available backups: ${c.before.frames[0].candidate_armour}; ${c.before.frames[0].candidate_weapon}.`;draw('before','Unfixed');draw('after','Corrected');$('raw-before').textContent=c.before.native_output||c.before.engine_messages.join('\n');$('raw-after').textContent=c.after.native_output||c.after.engine_messages.join('\n');}
$('case').value=mode;$('case').onchange=()=>{stop();mode=$('case').value;step=0;render()};$('step').oninput=()=>{stop();step=+$('step').value;render()};$('previous').onclick=()=>{stop();step=Math.max(0,step-1);render()};$('next').onclick=()=>{stop();step=Math.min(cases[mode].before.frames.length-1,step+1);render()};$('decision').onclick=()=>{stop();step=cases[mode].before.frames.findIndex(f=>f.stage==='Borg equipment decision applied');render()};$('damage').onclick=()=>{stop();step=cases[mode].before.frames.length-1;render()};$('play').onclick=()=>{if(timer){stop();return}if(step===cases[mode].before.frames.length-1)step=0;render();$('play').textContent='Pause';timer=setInterval(()=>{if(step===cases[mode].before.frames.length-1){stop();return}step++;render()},1600)};$('download').onclick=()=>{const a=document.createElement('a');a.href=URL.createObjectURL(new Blob([JSON.stringify(cases,null,2)],{type:'application/json'}));a.download='native-swap-replay-data.json';a.click();setTimeout(()=>URL.revokeObjectURL(a.href),1000)};render();
</script></html>'''
# Keep full readable native output beside the visual frames, without JSON frame lines.
for mode in rows:
 for phase in ['before','after']:
  output=(root/f'{phase}-{mode}.log').read_text()
  rows[mode][phase]['native_output']='\n'.join(line for line in output.splitlines() if not line.startswith('FRAME '))
payload=json.dumps(rows).replace('<','\\u003c')
(root/'replay.html').write_text(page.replace('__DATA__',payload))
(root/'replay-data.json').write_text(json.dumps(rows,indent=2)+'\n')
print(root/'replay.html')
