/**
 * §3.0.1's completeness rule, and the one distinction N-b exists to make.
 *
 * The rule: every setting an operator can change AT THE PANEL must have an editor in the menu. A pack
 * that omits one strands that setting, because the panel is the only surface some devices have.
 *
 * The distinction: a pack missing an editor for a setting that existed when it was authored is a BUG in
 * the pack. A pack missing an editor for a setting added to the firmware afterwards is not — it is an
 * old pack, and refusing it would mean every firmware release invalidated every pack on every card.
 * Those two look identical from the pack alone; telling them apart is what `sinceAbi` is for, and why
 * the owner decided on 2026-08-21 that every catalogue addition bumps `kUiCatalogueAbi`.
 *
 * ── WHAT A WARNING DOES NOT MEAN ─────────────────────────────────────────────────────────
 *
 * It does NOT mean the firmware will cover the gap. `Loadable_UI_Menu_Packs.md` §3.3.11a specifies a
 * load-time patcher that appends built-in editors for missing settings, and **that does not exist**.
 * So a warning today means precisely: this pack cannot reach that setting, and the operator needs the
 * portal, RS485 or a newer pack. It is information, not absolution. When §3.3.11a is built, the
 * warning becomes what it sounds like.
 *
 * Kept as plain ESM with no dependencies so the skeleton generator (`.mjs`, no build step) and the
 * exporter's tests can share ONE implementation. Two copies of an exemption list is precisely the
 * "one fact, two homes" failure this codebase keeps finding.
 */

/**
 * Settings that need a panel editor — i.e. §3.0.1's required set, after the two exemptions.
 *
 * Both exemptions are decided by a STATIC property of the value, never by runtime state, and that is
 * what makes them safe: the rule still proves that every setting an operator can change at the panel
 * has an editor there. Guarded editors were rejected for the opposite reason (R7.3) — a guard on
 * runtime state is not statically decidable.
 */
export function requiredPanelSettings(values) {
  return values.filter((v) => {
    if (v.category !== "setting") return false;
    // TEXT IS EXEMPT. There is no on-device text entry: three buttons and a 97-position character
    // wheel is not a usable way to type a 63-character WPA2 passphrase, and the owner ruled it out
    // (§6.3). Text reaches the device by the surfaces that were always the real ones — the portal
    // (§7.6), the RS485 block (§5.2) and the SD credential file (Q2).
    if (v.type === "string") return false;
    // NETWORK IS EXEMPT TOO, by the owner's decision that the panel only READS WiFi and MQTT
    // configuration. The same exemption widened to its natural edge: a panel that offers to edit half
    // a broker's settings is worse than one that offers none.
    if (v.id.startsWith("config.wifi.") || v.id.startsWith("config.mqtt.")) return false;
    return true;
  });
}

/**
 * Splits the uncovered required settings into failures and warnings.
 *
 * `covered` is the set of binding ids the pack (or skeleton) actually offers an editor for. `packAbi`
 * is the catalogue ABI the pack was stamped with. `sinceAbi` comes from the ledger, which is the only
 * record of when an id appeared — the manifest describes the catalogue as it is NOW and cannot answer
 * a question about history.
 *
 * A setting with no ledger entry is treated as `sinceAbi: 0`, i.e. as having always existed, so a
 * missing ledger line can only ever make this stricter. The ledger gate refuses that state anyway.
 */
export function classifyCoverage({ values, ledger, covered, packAbi }) {
  const sinceOf = new Map((ledger?.values ?? []).map((v) => [v.id, v.sinceAbi ?? 0]));
  const uncovered = requiredPanelSettings(values).filter((v) => !covered.has(v.id));
  const missing = [];
  const predating = [];
  for (const value of uncovered) {
    const since = sinceOf.get(value.id) ?? 0;
    // `>` and not `>=`: a pack stamped at the ABI a setting appeared in DID know about it.
    if (since > packAbi) predating.push({ id: value.id, sinceAbi: since });
    else missing.push(value.id);
  }
  return { missing, predating };
}
