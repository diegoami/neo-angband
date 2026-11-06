# Source comment cleanup

Current proposal head: `017ba47ebc8731fa0a61361a6f6777759a748d2a`. The proposal restores the original ordinary
comment; its complete C diff against the pristine base is now just the selection
condition:

```diff
-    if (b_p1 <= b_p2) {
+    if (weapon_swap && (!armour_swap || b_p2 <= b_p1)) {
```

[Preferred cumulative patch](../../../../patches/upstream-c/CBUG-0066-safer-swap-clean.patch).
The original verification/log receipt at66e10121 is preserved unchanged. Between
that tested revision and this head, only a source comment changed; executable
logic, scenarios, controls and measured79/26HP results are unchanged. No native
runtime gate was repeated for this comment-only cleanup. Hosted checks belong
to their exact head; the earlier passing run does not establish the new run.

Verification:

```sh
git diff 66e1012113fd7db634b19ffe86a470b58a7f7967 017ba47ebc8731fa0a61361a6f6777759a748d2a -- src/borg/borg-item-wear.c
git diff e473720ed88a0f0c7ca66815cef3000eb4a38bc3 017ba47ebc8731fa0a61361a6f6777759a748d2a -- src/borg/borg-item-wear.c
```

The first command shows only restoration of the original comment. The second
shows only the condition above. Internal catalogue labels belong in tracking
records, not in the submitted source comment.
