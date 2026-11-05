import { test } from "node:test";
import assert from "node:assert/strict";
import { runWatch } from "./borg-rs-watch.mjs";
const item={number:1,title:"C candidate",state:"OPEN",updatedAt:"now",url:"https://github.com/diegoami/borg-rs/issues/1"};
function io(document={sha:"blob",text:"## 1. Claim\n"},issues=[item]) {return {repo:()=>({default_branch:"main",has_issues:true}),head:()=>({sha:"head"}),document:()=>document,issues:()=>issues};}
test("reports changed file and new issues without promoting a lead",()=>{const lines=[];let saved;runWatch({io:io(),importedDigest:"old",save:(x)=>saved=x,report:(x)=>lines.push(x)});assert.equal(saved.mode,"file-and-issues");assert(lines.some(x=>x.includes("differs from")));assert(lines.some(x=>x.includes("classify C vs Rust-only")));});
test("supports file-to-issue migration without losing provenance",()=>{let saved;const lines=[];runWatch({io:io(null),previous:{document:{blob:"old"}},save:(x)=>saved=x,report:(x)=>lines.push(x)});assert.equal(saved.mode,"issues");assert(lines.some(x=>x.includes("Preserve file provenance")));});
test("does not save on API failure",()=>{let saves=0;const source=io();source.head=()=>{throw new Error("offline");};assert.throws(()=>runWatch({io:source,save:()=>saves++}),/offline/);assert.equal(saves,0);});
test("rejects missing feeds and truncated discovery",()=>{const source=io(null,[]);source.repo=()=>({default_branch:"main",has_issues:false});assert.throws(()=>runWatch({io:source,save:()=>{}}),/No C bug file/);assert.throws(()=>runWatch({io:io(null,Array(1000).fill(item)),save:()=>{}}),/window reached/);});
