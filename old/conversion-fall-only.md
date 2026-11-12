# Old conversion curve: fall only (superseded)

Status: **superseded on 2026-11-11** by the rise × fall curve ([`design-v2.md`](../design-v2.md)
§4.4.1).

## The old curve

```
g(L) = k × (1 + e^(−T/w)) / (1 + e^((L − T)/w))      // normalized logistic, g(0) = k
```

A fresh sub-skill converted at the full rate `k` from level 0; the conversion halved around `T`.

## Why it was replaced

- **Breadth bias.** Per unit of effort, breadth always won or tied: the cheap first levels of a
  new sub-skill were worth the full `k`. Ten sub-skills at 10 gave 30.6k main XP, about the
  same as four at 25 (34.1k) and close to half of one at 100 (69.9k at T=90, w=15).
- Countering this needed extra rules, listed as "possible tweaks" in §4.4.2: top-N, descending
  weights, a count cap per group. They are no longer needed.

## The new curve

`g(L) = k · smoothstep(L / T1) · fall(L)` with `T1 = 25` and the same `fall`. Same example:
4.0k / 18.3k / 66.0k. The old curve is the special case `T1 = 0` (in the script:
`[conversion] rise_T1 = 0`).
