"""Build report.html (a self-contained page: inline SVG charts, theme-aware) from the result files."""
import json, os, re, sys, html

sys.path.insert(0, os.path.dirname(os.path.abspath(__file__)))
from readme_data import README_WORST, README_ALTS

ROOT = os.path.dirname(os.path.dirname(os.path.abspath(__file__)))
RES = os.path.join(ROOT, "results")


def J(name, default=None):
    p = os.path.join(RES, name)
    return json.load(open(p)) if os.path.exists(p) else default


def fs(seq):
    return "{" + ",".join(map(str, seq)) + "}"


def esc(s):
    return html.escape(str(s))


# ---------------------------------------------------------------- charts (inline SVG, to scale)
def nice_ticks(lo, hi, n=5):
    import math
    span = hi - lo
    raw = span / n
    mag = 10 ** math.floor(math.log10(raw))
    step = min((s * mag for s in (1, 2, 2.5, 5, 10) if s * mag >= raw), default=10 * mag)
    start = math.floor(lo / step) * step
    ticks, t = [], start
    while t <= hi + 1e-9:
        if t >= lo - 1e-9:
            ticks.append(round(t, 6))
        t += step
    return ticks


def line_chart(series, xlab, ylab, w=640, h=300, ylo=None, yhi=None):
    """series: list of (label, cssvar, [(x,y)...], style) style in {'line','dots','line+dots'}"""
    xs = [x for _, _, pts, _ in series for x, _ in pts]
    ys = [y for _, _, pts, _ in series for _, y in pts]
    x0, x1 = min(xs), max(xs)
    y0 = ylo if ylo is not None else min(ys)
    y1 = yhi if yhi is not None else max(ys)
    yt = nice_ticks(y0, y1)
    y0, y1 = min(y0, yt[0]), max(y1, yt[-1])
    L, R, T, B = 52, 16, 14, 40
    px = lambda x: L + (x - x0) / (x1 - x0) * (w - L - R)
    py = lambda y: T + (1 - (y - y0) / (y1 - y0)) * (h - T - B)
    out = [f'<svg viewBox="0 0 {w} {h}" role="img" class="chart">']
    for t in yt:
        out.append(f'<line x1="{L}" x2="{w-R}" y1="{py(t):.1f}" y2="{py(t):.1f}" class="grid"/>')
        out.append(f'<text x="{L-6}" y="{py(t)+4:.1f}" class="tick" text-anchor="end">{t:g}</text>')
    for t in nice_ticks(x0, x1, 8):
        out.append(f'<text x="{px(t):.1f}" y="{h-B+16}" class="tick" text-anchor="middle">{t:g}</text>')
    out.append(f'<text x="{(L+w-R)/2:.0f}" y="{h-6}" class="axis" text-anchor="middle">{esc(xlab)}</text>')
    out.append(f'<text x="14" y="{(T+h-B)/2:.0f}" class="axis" text-anchor="middle" transform="rotate(-90 14 {(T+h-B)/2:.0f})">{esc(ylab)}</text>')
    for label, var, pts, style in series:
        pts = sorted(pts)
        if "line" in style:
            d = " ".join(f"{'M' if i == 0 else 'L'}{px(x):.1f},{py(y):.1f}" for i, (x, y) in enumerate(pts))
            out.append(f'<path d="{d}" fill="none" stroke="var({var})" stroke-width="2"/>')
        if "dots" in style:
            for x, y in pts:
                out.append(f'<circle cx="{px(x):.1f}" cy="{py(y):.1f}" r="3.2" fill="var({var})"><title>{esc(label)} N={x}: {y:g}</title></circle>')
        if "ring" in style:
            for x, y in pts:
                out.append(f'<circle cx="{px(x):.1f}" cy="{py(y):.1f}" r="6" fill="none" stroke="var({var})" stroke-width="1.5"><title>{esc(label)} N={x}: {y:g}</title></circle>')
    out.append("</svg>")
    legend = "".join(f'<span class="key"><i style="background:var({v})"></i>{esc(l)}</span>' for l, v, _, _ in series)
    return f'<figure class="fig">{"".join(out)}<figcaption class="legend">{legend}</figcaption></figure>'


def bar_chart(items, xlab, w=640, h=None, fmt="{:+d}"):
    """items: list of (label, value, cssvar) horizontal bars around zero"""
    h = h or 28 + 26 * len(items)
    vals = [v for _, v, _ in items]
    lo, hi = min(0, min(vals)), max(0, max(vals))
    L, R = 150, 56
    px = lambda v: L + (v - lo) / ((hi - lo) or 1) * (w - L - R)
    out = [f'<svg viewBox="0 0 {w} {h}" role="img" class="chart">']
    out.append(f'<line x1="{px(0):.1f}" x2="{px(0):.1f}" y1="6" y2="{h-22}" class="axisline"/>')
    for i, (lab, v, var) in enumerate(items):
        y = 10 + 26 * i
        a, b = sorted((px(0), px(v)))
        out.append(f'<text x="{L-8}" y="{y+13}" class="tick" text-anchor="end">{esc(lab)}</text>')
        out.append(f'<rect x="{a:.1f}" y="{y}" width="{max(b-a,1.5):.1f}" height="18" rx="2" fill="var({var})"/>')
        out.append(f'<text x="{(b if v >= 0 else a) + (6 if v >= 0 else -6):.1f}" y="{y+13}" class="val" text-anchor="{"start" if v >= 0 else "end"}">{fmt.format(v)}</text>')
    out.append(f'<text x="{(L+w-R)/2:.0f}" y="{h-4}" class="axis" text-anchor="middle">{esc(xlab)}</text>')
    out.append("</svg>")
    return f'<figure class="fig">{"".join(out)}</figure>'


def stacked_bars(rows, w=640):
    """rows: list of (label, [(gap, count), ...]) -> horizontal stacked bars of comparisons per pass"""
    h = 18 + 30 * len(rows)
    total_max = max(sum(c for _, c in parts) for _, parts in rows)
    L, R = 120, 60
    sc = (w - L - R) / total_max
    vars_ = ["--p1", "--p2", "--p3", "--p4", "--p5", "--p6", "--p7"]
    out = [f'<svg viewBox="0 0 {w} {h}" role="img" class="chart">']
    for i, (lab, parts) in enumerate(rows):
        y = 8 + 30 * i
        x = L
        out.append(f'<text x="{L-8}" y="{y+14}" class="tick" text-anchor="end">{esc(lab)}</text>')
        k = len(parts)
        for j, (g, c) in enumerate(parts):
            var = vars_[min(k - 1 - j, len(vars_) - 1)]       # final pass always the same color
            wd = c * sc
            out.append(f'<rect x="{x:.1f}" y="{y}" width="{wd:.1f}" height="20" fill="var({var})"><title>gap {g}: {c} comparisons</title></rect>')
            if wd > 26:
                out.append(f'<text x="{x+wd/2:.1f}" y="{y+14}" class="inbar" text-anchor="middle">{c}</text>')
            x += wd
        out.append(f'<text x="{x+6:.1f}" y="{y+14}" class="val">{sum(c for _, c in parts)}</text>')
    out.append("</svg>")
    return f'<figure class="fig">{"".join(out)}<figcaption class="legend"><span class="key"><i style="background:var(--p1)"></i>final pass (gap 1)</span><span class="key"><i style="background:var(--p2)"></i>gap 4</span><span class="key"><i style="background:var(--p3)"></i>third-smallest gap</span><span class="key"><i style="background:var(--p4)"></i>larger gaps</span></figcaption></figure>'


# ---------------------------------------------------------------- page
def build():
    summ = J("summary.json", {})
    anat = J("anatomy.json", {})
    tr = J("transfer.json", {})
    st = J("strong.json", {})
    keys = sorted(summ, key=int)

    # main table
    rows, n_match, n_total, corrections = [], 0, 0, []
    for k in keys:
        s = summ[k]; n = s["N"]
        if not s["optimal"]:
            continue
        b = s["optimal"][0]
        rd = README_WORST.get(n)
        n_total += 1 if rd else 0
        oset = {tuple(r["seq"]) for r in s["optimal"]}
        rset = ({tuple(rd[0])} | {tuple(a) for a in README_ALTS.get(n, [])}) if rd else set()
        status, cls = "matches", "ok"
        if rd and rd[1] == s["W"]:
            n_match += 1
            if tuple(b["seq"]) != tuple(rd[0]):
                status, cls = "better tie-break", "new"
            elif oset != rset:
                status, cls = "tie list corrected", "new"
        elif rd:
            status, cls = f"README {rd[1]}", "warn"
        else:
            status, cls = "new (beyond README)", "new"
        if rd and oset != rset:
            new = sorted(oset - rset, key=lambda t: (len(t), t)); bad = sorted(rset - oset, key=lambda t: (len(t), t))
            corrections.append((n, s["W"], new, bad))
        ties = ", ".join(fs(r["seq"]) for r in s["optimal"][1:])
        rows.append(f'<tr><td class="num">{n}</td><td class="num"><b>{s["W"]}</b></td><td class="seq">{fs(b["seq"])}</td>'
                    f'<td class="num">{("≈ " + format(b["avg"], ".2f")) if b.get("avg_kind") == "sampled" else format(b["avg"], ".3f")}</td>'
                    f'<td class="num">{len(s["optimal"])}</td>'
                    f'<td><span class="pill {cls}">{status}</span></td><td class="seq small">{ties}</td></tr>')
    main_table = ('<div class="scroll"><table><thead><tr><th>N</th><th>W*</th><th>optimal sequence</th><th>average (≈ sampled)</th>'
                  '<th>ties</th><th>vs README</th><th>other optimal sequences (ties)</th></tr></thead><tbody>'
                  + "".join(rows) + "</tbody></table></div>")

    corr_items = []
    for n, W, new, bad in corrections:
        parts = []
        if new:
            parts.append("also optimal: " + ", ".join(f'<span class="seq">{fs(t)}</span>' for t in new))
        if bad:
            parts.append("listed but not optimal: " + ", ".join(f'<span class="seq">{fs(t)}</span>' for t in bad))
        corr_items.append(f"<li><b>N = {n}</b> (W* = {W}): " + "; ".join(parts) + "</li>")

    # charts
    exact_pts = [(summ[k]["N"], summ[k]["W"] / summ[k]["N"]) for k in keys if summ[k]["optimal"]]
    rd_pts = [(n, v[1] / n) for n, v in README_WORST.items() if n <= 45]
    chart1 = line_chart([("exact optimum (this work)", "--accent", exact_pts, "line+dots"),
                         ("README value", "--muted", rd_pts, "ring")],
                        "array size N", "W* / N")
    cross = {}
    p = os.path.join(RES, "core_crossover.txt")
    if os.path.exists(p):
        for line in open(p):
            m = re.match(r"N=(\d+)\s+\{1,4,7,9\}=(\d+)\s+\{1,4,9,11\}=(\d+)", line.strip())
            if m:
                cross[int(m.group(1))] = (int(m.group(2)), int(m.group(3)))
    chart2 = bar_chart([(f"N = {n}", a - b, "--accent" if a - b > 0 else "--muted") for n, (a, b) in sorted(cross.items())],
                       "W{1,4,7,9} − W{1,4,9,11}   (positive: {1,4,9,11} is better)") if cross else ""
    anat_rows = []
    for n in (16, 24, 32, 36, 40, 45):
        a = anat.get(str(n))
        if a:
            anat_rows.append((f"N={n} {fs(a['seq'])}", [tuple(x) for x in a["per_pass"]]))
    chart3 = stacked_bars(anat_rows) if anat_rows else ""

    # large N: every candidate's best stored (re-simulated) lower bound, from all sources
    from make_report import heavily_attacked
    dv = J("transfer_div.json", {}); d15 = J("transfer_15.json", {})
    lrows, lbars = [], []
    for n in (64, 91, 128, 181, 256, 362, 512):
        rd = README_WORST.get(n)
        if not rd:      # beyond the README table (N = 512): this work's best bound only
            cand = {tuple(int(x) for x in k.split(":")[1].split(",")): v["W_lower"] for k, v in st.items()
                    if int(k.split(":")[0]) == n}
            heavy = heavily_attacked(n)
            best = sorted((w, len(t), t) for t, w in cand.items() if max(t) <= n // 2 and (not heavy or t in heavy))
            if best:
                lrows.append(f'<tr><td class="num">{n}</td><td>– (table ends at N = 362)</td><td class="num">–</td>'
                             f'<td class="num">–</td><td class="seq">{fs(best[0][2])}</td><td class="num">{best[0][0]}</td>'
                             f'<td><span class="pill ok">no README sequence</span></td></tr>')
            continue
        cand = {}
        for db in (tr, dv, d15):
            d = db.get(str(n))
            if d:
                for r in d["final"] + ([d["readme"]] if d.get("readme") else []):
                    t = tuple(r["seq"]); cand[t] = max(cand.get(t, 0), r["W_lower"])
        for key, v in st.items():
            N, sq = key.split(":")
            if int(N) == n:
                t = tuple(int(x) for x in sq.split(",")); cand[t] = max(cand.get(t, 0), v["W_lower"])
        if n >= 91:     # reference convention for N >= 91: gaps <= N/2 only
            cand = {t: w for t, w in cand.items() if max(t) <= n // 2}
        rw = cand.get(tuple(rd[0]))
        heavy = heavily_attacked(n)     # only sequences attacked as hard as the leaders, when such rounds exist
        others = sorted((w, len(t), t) for t, w in cand.items() if t != tuple(rd[0]) and (not heavy or t in heavy))
        bw, _, best = others[0] if others else (None, 0, None)
        if bw is None or rw is None:
            verdict, vcls = "–", "ok"
        elif bw > rw:
            verdict, vcls = "README sequence best known", "ok"
        elif bw == rw:
            verdict, vcls = "tie (no improvement)", "ok"
        else:
            verdict, vcls = "other sequence lower (lower bounds only)", "warn"
        lrows.append(f'<tr><td class="num">{n}</td><td class="seq">{fs(rd[0])}</td><td class="num">{rd[1]}</td>'
                     f'<td class="num"><b>{rw if rw else "–"}</b></td><td class="seq">{fs(best) if best else "–"}</td>'
                     f'<td class="num">{bw if bw else "–"}</td><td><span class="pill {vcls}">{verdict}</span></td></tr>')
        if rw:
            lbars.append((f"N = {n}", round(100.0 * (rw - rd[1]) / rd[1], 1), "--warn"))
    # data-driven large-N finding
    under = []
    for n in (64, 91, 128, 181, 256, 362):
        rd = README_WORST.get(n)
        vals = [v["W_lower"] for k, v in st.items() if int(k.split(":")[0]) == n and [int(x) for x in k.split(":")[1].split(",")] == rd[0]]
        t = tr.get(str(n))
        if t and t.get("readme"):
            vals.append(t["readme"]["W_lower"])
        if vals and max(vals) > rd[1]:
            under.append((n, max(vals), rd[1]))
    if under:
        worst = max(under, key=lambda u: (u[1] - u[2]) / u[2])
        large_finding = (f"For the reference's own sequences this work finds inputs that cost more than the published worst case at "
                         f"N = {', '.join(str(u[0]) for u in under)} (largest gap: N = {worst[0]}, ≥ {worst[1]} vs {worst[2]}, "
                         f"+{100.0 * (worst[1] - worst[2]) / worst[2]:.1f}%). Those published values are therefore too low.")
    else:
        large_finding = "Large-N comparison pending."
    large_table = ('<div class="scroll"><table><thead><tr><th>N</th><th>README sequence</th><th>README worst case</th>'
                   '<th>input found here (≥)</th><th>best other sequence found here</th><th>its worst case (≥)</th><th>verdict</th></tr></thead><tbody>'
                   + "".join(lrows) + "</tbody></table></div>") if lrows else "<p>(large-N runs pending)</p>"
    chart4 = bar_chart(lbars, "how far above the README value our worst input goes (%)", fmt="{:+.1f}%") if lbars else ""

    from make_report import race_verdicts, campaign_lines
    rv, cv = [], []
    for out_, lines in ((rv, race_verdicts()), (cv, campaign_lines())):
        for line in lines:
            t = esc(line.lstrip("* ").strip())
            t = re.sub(r"\{([\d,]+)\}", r'<span class="seq">{\1}</span>', t)
            t = re.sub(r"^N = (\d+):", r"<b>N = \1:</b>", t)
            out_.append(f"<li>{t}</li>")

    beyond = []
    for k in keys:
        s = summ[k]
        if s["N"] not in README_WORST and s["optimal"]:
            seqs = ", ".join(fs(r["seq"]) for r in s["optimal"])
            possible = [r for r in s.get("unresolved", []) if r["W"] >= s["W"]]
            proven = all(r["W"] >= s["W"] for r in s.get("unresolved", []))
            killed = [r for r in s.get("certified_above", []) if r.get("killed")]
            ruled = [r for r in s["optimal"] if "front pair rules" in str(r.get("certified", ""))]
            extra = (f' {" and ".join(fs(r["seq"]) for r in ruled)} {"were" if len(ruled) > 1 else "was"} certified with a '
                     f'DP that turns leading pair passes into penalties on a smaller state space; annealing alone had '
                     f'stalled at {s["W"]} for them.' if ruled else "")
            extra += (f' {len(possible)} more sequences are ≥ {s["W"]}; targeted annealing never pushed them to {s["W"] + 1}, '
                      f'but the exact DP for them does not fit in this machine\'s memory. All have higher averages.'
                     if possible else
                     f' The tie list is complete: the {len(killed)} candidates too large for the exact DP were each pushed above '
                     f'{s["W"]} by an explicit input found by targeted annealing.' if killed else "")
            beyond.append(f'<li><b>Beyond the reference, N = {s["N"]}.</b> W* = {s["W"]}{" (proved)" if proven else ""}, '
                          f'attained by <span class="seq">{seqs}</span>; the best by average is '
                          f'<span class="seq">{fs(s["optimal"][0]["seq"])}</span>.{extra}</li>')

    # README average-optimal sequences: their worst case, exact or lower bound, and its price vs W*
    from make_report import avgopt_data, avgopt_finding, avgopt_status
    arows = avgopt_data()
    trs = []
    for r in arows:
        price, here = "–", "–"
        if r["W"] is not None:
            here = ("" if r["exact"] else "≥ ") + str(r["W"])
            if r["wstar"]:
                price = ("" if r["wstar_exact"] and r["exact"] else "≈ ") + f"+{100.0 * (r['W'] - r['wstar']) / r['wstar']:.1f}%"
        st_ = avgopt_status(r)
        cls = ("ok" if "matches" in st_ or "reached" == st_.split()[-1] or "no README" in st_ else
               "new" if "improved" in st_ or "too low" in st_ else "warn")
        avg = "–" if r["avg"] is None else f"≈ {r['avg']:.2f} (here)" if r.get("avg_here") else str(r["avg"])
        rw = "– (not in README)" if r["readme"] is None else f'{"≥ " if r["readme_lb"] else ""}{r["readme"]}'
        trs.append(f'<tr><td class="num">{r["n"]}</td><td class="seq">{fs(r["seq"])}</td><td class="num">{avg}</td>'
                   f'<td class="num">{rw}</td><td class="num"><b>{here}</b></td>'
                   f'<td><span class="pill {cls}">{esc(st_)}</span></td><td class="num">{price}</td></tr>')
    avgopt_table = ('<div class="scroll"><table><thead><tr><th>N</th><th>README best-average sequence</th><th>README average</th>'
                    '<th>README worst case</th><th>worst case here</th><th>status</th><th>vs optimal worst case</th></tr></thead>'
                    '<tbody>' + "".join(trs) + "</tbody></table></div>")
    ex = [r for r in arows if r["W"] is not None and r["exact"] and r["wstar"]]
    avgopt_chart = line_chart(
        [("worst case of the average-optimal sequence, % above the optimal worst case", "--warn",
          [(r["n"], round(100.0 * (r["W"] - r["wstar"]) / r["wstar"], 2)) for r in ex], "line+dots"),
         ("average of the worst-case-optimal sequence, % above the best average", "--accent",
          [(r["n"], round(100.0 * (r["wopt_avg"] - r["avg"]) / r["avg"], 2)) for r in ex], "line+dots")],
        "N", "% above the optimum", ylo=0) if ex else ""
    avgopt_finding_txt = esc(avgopt_finding(arows)) if any(r["W"] is not None for r in arows) else "Pending."

    tpl = open(os.path.join(ROOT, "scripts", "report_template.html"), encoding="utf-8").read()
    rep = {"MAIN_TABLE": main_table, "CORRECTIONS": "".join(corr_items) or "<li>none</li>", "CHART_PER_ELEMENT": chart1,
           "CHART_CROSSOVER": chart2, "CHART_ANATOMY": chart3, "LARGE_TABLE": large_table, "CHART_LARGE": chart4,
           "N_MATCH": str(n_match), "N_TOTAL": str(n_total), "N_LAST": keys[-1] if keys else "?",
           "N_LAST_README": str(max(int(k) for k in keys if int(k) in README_WORST)) if keys else "?",
           "LARGE_FINDING": large_finding, "BEYOND": "".join(beyond), "RACE_VERDICTS": "".join(rv),
           "CAMPAIGN": "".join(cv) or "<li>(pending)</li>",
           "AVGOPT_TABLE": avgopt_table, "AVGOPT_CHART": avgopt_chart, "AVGOPT_FINDING": avgopt_finding_txt}
    for k, v in rep.items():
        tpl = tpl.replace("{{" + k + "}}", v)
    out = os.path.join(ROOT, "report.html")
    open(out, "w", encoding="utf-8").write(tpl)
    print("wrote", out, len(tpl), "bytes")


if __name__ == "__main__":
    build()
