#!/usr/bin/env python3
"""Generate docs/datapath.drawio.svg for the Tang Nano 20K MIPS core.

Emits a single file that is both a valid SVG (renders inline on GitHub) and a
draw.io document (the `content` attribute on <svg> holds the mxGraph XML, which
hediet.vscode-drawio opens for editing). Both halves come from the one SPEC /
EDGES description below, so they cannot drift apart.

Three things keep the picture readable, and all three are enforced here rather
than eyeballed:

* Box widths are computed from their own text, so a label can never spill out.
* Every wire is an explicit orthogonal route through a gutter no block occupies,
  and check_collisions() fails the run if a segment crosses a block it does not
  belong to, naming the segment.
* Arrowheads are drawn in user space and each line is trimmed by exactly the
  head length, so the triangle sits on the end of the segment, never over it.

The I2C programming and readback path is deliberately NOT drawn here: the
Mermaid system diagram in ../README.md already shows it, and repeating it cost
this picture a whole row it needed for the hazard control.
"""
import sys
from xml.sax.saxutils import escape

W, H = 1680, 900
HEAD = 10                      # arrowhead length, in user units

PAL = {
    "pred": ("#FDF0E3", "#C08035"),
    "if":   ("#E8F0FE", "#3C6098"),
    "mem":  ("#E6F4EA", "#41805A"),
    "ex":   ("#E8F0FE", "#3C6098"),
    "ctrl": ("#FCE8E6", "#BC4A40"),
}
BAR_FILL, BAR_STROKE = "#3C6098", "#28405F"
INK = "#16212E"
FEED = "#BC4A40"     # forwarding and write-back returns
PRED = "#C08035"     # branch prediction and its update
CTRL = "#9A5BA8"     # stall / flush control
WIRE = "#3C6098"     # forward datapath

LBL_CH, SUB_CH = 7.3, 5.1      # approximate advance width per character

# id, left x, top y, kind, label, sublabel -- width and height are computed
SPEC = [
    ("pht",   40, 104, "pred", "PHT", "16 x 4"),
    ("xorr", 150, 104, "pred", "Xor_result", "hist ^ PC"),
    ("bht",  300, 104, "pred", "BHT", "16 x 1"),
    ("btb",  420, 104, "pred", "BTB", "16 x 32"),

    ("mux1",  36, 240, "if",  "mux_1", "BHT ? BTB : PC+4"),
    ("mux2",  36, 330, "if",  "mux_2", "branch ? pred : PC+4"),
    ("pc",    36, 420, "if",  "Pc", "PC_out / PC_next"),
    ("imem", 200, 420, "mem", "instruction_memory", "256 x 32 BSRAM"),
    ("cmp",  200, 510, "if",  "comparator", "beq? -> mux_2 sel"),

    ("cu",   480, 240, "ex",  "control_unit", "opcode only"),
    ("rf",   480, 420, "mem", "reg_file", "32 x 32 flops"),
    ("se",   480, 510, "ex",  "Sign_extender", "16 -> 32"),

    ("fwdu",  720, 240, "ctrl", "Forwarding_unit", "picks each operand"),
    ("frs",   720, 330, "ex",   "Forward_rs", "3:1 mux"),
    ("frt",   720, 420, "ex",   "forward_rt", "3:1 mux"),
    ("mux1e", 880, 420, "ex",   "mux_1_execution", "ALUSrc"),
    ("alu",  1060, 375, "ex",   "mips_alu", "add / sub"),
    ("mux2e", 880, 510, "ex",   "mux_2_execution", "branch target"),
    ("mux3e",1060, 510, "ex",   "mux_3_execution", "RegDst"),

    ("dmem", 1300, 405, "mem", "data_memory", "256 x 32 BSRAM"),
    ("wbm",  1510, 420, "ex",  "Write_back_mux", "MemtoReg"),

    ("stall", 200, 660, "ctrl", "stalling_unit", "load-use hazard"),
    ("flush", 480, 660, "ctrl", "Flushing_unit", "mispredict"),
    ("brnch",1060, 660, "ctrl", "branching_unit", "predictor write"),
]

BARS = [("ifid", 410, "IF/ID"), ("idex", 660, "ID/EX"),
        ("exmem", 1240, "EX/MEM"), ("memwb", 1450, "MEM/WB")]
BAR_Y, BAR_W, BAR_H = 210, 28, 390        # 210 .. 600

STAGES = [(195, "IF — fetch"), (545, "ID — decode"), (960, "EX — execute"),
          (1351, "MEM"), (1572, "WB")]

BOX = {}
for _id, _x, _y, _k, _l, _s in SPEC:
    _w = max(66, len(_l) * LBL_CH + 24, len(_s) * SUB_CH + 24)
    BOX[_id] = (_id, _x, _y, round(_w), 50, _k, _l, _s)
for _id, _x, _l in BARS:
    BOX[_id] = (_id, _x, BAR_Y, BAR_W, BAR_H, "bar", _l, "")


def g(bid):
    return BOX[bid][1], BOX[bid][2], BOX[bid][3], BOX[bid][4]


def L(bid, y):
    x, _, _, _ = g(bid)
    return (x, y)


def R(bid, y):
    x, _, w, _ = g(bid)
    return (x + w, y)


def T(bid, x):
    _, y, _, _ = g(bid)
    return (x, y)


def B(bid, x):
    _, y, _, h = g(bid)
    return (x, y + h)


# ---------------------------------------------------------------------- wires
# Routed by hand through gutters; check_collisions() proves none cuts a block.
EDGES = [
    # PC[5:2] out to the predictor tables (one bus, drawn as two branches)
    ("pc", "pht", PRED, [L("pc", 445), (16, 445), (16, 184), (55, 184), T("pht", 55)]),
    ("pc", "btb", PRED, [L("pc", 445), (16, 445), (16, 184), (432, 184), T("btb", 432)]),
    # the predictor chain itself
    ("pht", "xorr", PRED, [R("pht", 129), L("xorr", 129)]),
    ("xorr", "bht", PRED, [R("xorr", 129), L("bht", 129)]),
    ("bht", "mux1", PRED, [B("bht", 340), (340, 194), (70, 194), T("mux1", 70)]),
    ("btb", "mux1", PRED, [B("btb", 452), (452, 204), (110, 204), T("mux1", 110)]),
    # fetch
    ("mux1", "mux2", WIRE, [B("mux1", 88), T("mux2", 88)]),
    ("mux2", "pc", WIRE, [B("mux2", 88), T("pc", 88)]),
    ("pc", "imem", WIRE, [R("pc", 445), L("imem", 445)]),
    ("imem", "cmp", WIRE, [B("imem", 266), T("cmp", 266)]),
    ("cmp", "mux2", WIRE, [R("cmp", 535), (352, 535), (352, 592), (176, 592),
                           (176, 355), L("mux2", 355)]),
    ("imem", "ifid", WIRE, [R("imem", 445), L("ifid", 445)]),
    # decode
    ("ifid", "cu", WIRE, [R("ifid", 265), L("cu", 265)]),
    ("ifid", "rf", WIRE, [R("ifid", 445), L("rf", 445)]),
    ("ifid", "se", WIRE, [R("ifid", 535), L("se", 535)]),
    ("cu", "idex", WIRE, [R("cu", 265), L("idex", 265)]),
    ("rf", "idex", WIRE, [R("rf", 445), L("idex", 445)]),
    ("se", "idex", WIRE, [R("se", 535), L("idex", 535)]),
    # execute
    ("idex", "frs", WIRE, [R("idex", 355), L("frs", 355)]),
    ("idex", "frt", WIRE, [R("idex", 445), L("frt", 445)]),
    ("idex", "mux1e", WIRE, [R("idex", 492), (852, 492), (852, 458), L("mux1e", 458)]),
    ("idex", "mux2e", WIRE, [R("idex", 540), L("mux2e", 540)]),
    ("idex", "mux3e", WIRE, [R("idex", 588), (1036, 588), (1036, 545), L("mux3e", 545)]),
    ("fwdu", "frs", CTRL, [B("fwdu", 762), T("frs", 762)]),
    ("idex", "fwdu", CTRL, [R("idex", 265), L("fwdu", 265)]),
    ("fwdu", "frt", CTRL, [R("fwdu", 265), (862, 265), (862, 452), R("frt", 452)]),
    ("frs", "alu", WIRE, [R("frs", 355), (1032, 355), (1032, 392), L("alu", 392)]),
    ("frt", "mux1e", WIRE, [R("frt", 435), L("mux1e", 435)]),
    ("mux1e", "alu", WIRE, [R("mux1e", 445), (1032, 445), (1032, 412), L("alu", 412)]),
    ("alu", "mux2e", WIRE, [B("alu", 1100), (1100, 486), (946, 486), T("mux2e", 946)]),
    ("alu", "exmem", WIRE, [R("alu", 400), L("exmem", 400)]),
    ("mux3e", "exmem", WIRE, [R("mux3e", 535), (1216, 535), (1216, 470), L("exmem", 470)]),
    # memory and write-back
    ("exmem", "dmem", WIRE, [R("exmem", 430), L("dmem", 430)]),
    ("exmem", "memwb", WIRE, [R("exmem", 260), L("memwb", 260)]),
    ("memwb", "wbm", WIRE, [R("memwb", 445), L("wbm", 445)]),
    ("dmem", "wbm", WIRE, [B("dmem", 1351), (1351, 628), (1572, 628), B("wbm", 1572)]),
    # write-back and forwarding returns
    ("wbm", "rf", FEED, [B("wbm", 1600), (1600, 742), (398, 742), (398, 470), B("rf", 398)]),
    ("wbm", "frs", FEED, [(1600, 742), (704, 742), (704, 382), L("frs", 382)]),
    ("wbm", "frt", FEED, [(1600, 742), (762, 742), B("frt", 762)]),
    ("exmem", "frs", FEED, [B("exmem", 1254), (1254, 782), (696, 782), (696, 368),
                            L("frs", 368)]),
    ("exmem", "frt", FEED, [(1254, 782), (792, 782), B("frt", 792)]),
    # PC_calculated: redirect, flush and predictor update
    ("mux2e", "flush", PRED, [B("mux2e", 946), (946, 822), (538, 822), B("flush", 538)]),
    ("mux2e", "brnch", PRED, [(946, 822), (1122, 822), B("brnch", 1122)]),
    ("mux2e", "pc", PRED, [(946, 822), (88, 822), B("pc", 88)]),
    ("mux2e", "btb", PRED, [(946, 822), (392, 822), (392, 164), (432, 164), B("btb", 432)]),
    # branching_unit writes the three predictor tables
    ("brnch", "pht", PRED, [T("brnch", 1090), (1090, 640), (368, 640), (368, 174),
                            (70, 174), B("pht", 70)]),
    ("brnch", "bht", PRED, [(368, 174), (315, 174), B("bht", 315)]),
    ("brnch", "btb", PRED, [(368, 174), (472, 174), B("btb", 472)]),
    # hazard control: what each unit watches, and what it holds or clears
    ("idex", "stall", CTRL, [B("idex", 676), (676, 626), (262, 626), T("stall", 262)]),
    ("stall", "pc", CTRL, [T("stall", 220), (220, 610), (100, 610), B("pc", 100)]),
    ("stall", "ifid", CTRL, [T("stall", 242), (242, 617), (424, 617), B("ifid", 424)]),
    ("stall", "idex", CTRL, [T("stall", 292), (292, 604), (670, 604), B("idex", 670)]),
    ("flush", "pc", CTRL, [T("flush", 502), (502, 650), (124, 650), B("pc", 124)]),
    ("flush", "ifid", CTRL, [T("flush", 532), (532, 643), (432, 643), B("ifid", 432)]),
    ("flush", "idex", CTRL, [T("flush", 572), (572, 636), (682, 636), B("idex", 682)]),
]

NOTES = [
    (23, 232, PRED, "PC[5:2]", "start"),
    (410, 736, FEED, "write_data — register-file write, and the MEM/WB forward", "start"),
    (410, 776, FEED, "EX_MEM_R — the EX/MEM forward", "start"),
    (410, 816, PRED, "PC_calculated — redirect, flush, predictor update", "start"),
    (1351, 644, WIRE, "load result skips MEM/WB", "middle"),
]


def trim(pts):
    """Shorten the last segment by the arrowhead length."""
    (x1, y1), (x2, y2) = pts[-2], pts[-1]
    dx, dy = x2 - x1, y2 - y1
    n = (dx * dx + dy * dy) ** 0.5
    if n <= HEAD:
        return pts
    return pts[:-1] + [(x2 - dx / n * HEAD, y2 - dy / n * HEAD)]


def check_collisions():
    bad = []
    for src, dst, _c, pts in EDGES:
        for (x1, y1), (x2, y2) in zip(pts, pts[1:]):
            for bid in BOX:
                if bid in (src, dst):
                    continue
                _, bx, by, bw, bh, *_ = BOX[bid]
                if max(x1, x2) > bx + 2 and min(x1, x2) < bx + bw - 2 and \
                   max(y1, y2) > by + 2 and min(y1, y2) < by + bh - 2:
                    bad.append(f"{src}->{dst}: ({x1},{y1})-({x2},{y2}) crosses {bid}")
    return bad


def build_svg():
    p = ['<?xml version="1.0" encoding="UTF-8"?>']
    heads = "".join(
        f'<marker id="h{c[1:]}" markerUnits="userSpaceOnUse" markerWidth="{HEAD}" '
        f'markerHeight="8" refX="0" refY="4" orient="auto">'
        f'<polygon points="0,0 {HEAD},4 0,8" fill="{c}"/></marker>'
        for c in (WIRE, FEED, PRED, CTRL))
    p.append(f'<svg xmlns="http://www.w3.org/2000/svg" width="{W}" height="{H}" '
             f'viewBox="0 0 {W} {H}" content="@@CONTENT@@">')
    p.append(f'<defs>{heads}</defs>')
    p.append(f'<rect width="{W}" height="{H}" fill="#FFFFFF"/>')
    p.append(f'<text x="24" y="36" font-family="sans-serif" font-size="18" font-weight="bold" '
             f'fill="{INK}">MIPS_proc datapath — 5 stages, forwarding, two-level branch '
             f'prediction</text>')
    p.append(f'<text x="24" y="58" font-family="sans-serif" font-size="11.5" fill="#5A6B7D">'
             f'Every box is a Verilog module in src/.  '
             f'<tspan fill="{WIRE}" font-weight="bold">Blue</tspan> = forward datapath,  '
             f'<tspan fill="{FEED}" font-weight="bold">red</tspan> = values fed backwards,  '
             f'<tspan fill="{PRED}" font-weight="bold">orange</tspan> = branch prediction and '
             f'its update,  <tspan fill="{CTRL}" font-weight="bold">purple</tspan> = hazard '
             f'control.   I2C programming is in the system diagram in README.md.</text>')

    for x, label in STAGES:
        p.append(f'<text x="{x}" y="234" text-anchor="middle" font-family="sans-serif" '
                 f'font-size="12.5" font-weight="bold" fill="#5A6B7D">{escape(label)}</text>')

    for bid, x, label in BARS:
        p.append(f'<rect x="{x}" y="{BAR_Y}" width="{BAR_W}" height="{BAR_H}" rx="3" '
                 f'fill="{BAR_FILL}" stroke="{BAR_STROKE}" stroke-width="1.5"/>')
        p.append(f'<text x="{x+BAR_W/2}" y="{BAR_Y+BAR_H/2}" text-anchor="middle" '
                 f'font-family="sans-serif" font-size="12" font-weight="bold" fill="#FFFFFF" '
                 f'transform="rotate(-90 {x+BAR_W/2} {BAR_Y+BAR_H/2})">{escape(label)}</text>')

    for bid in BOX:
        _, x, y, w, h, kind, label, sub = BOX[bid]
        if kind == "bar":
            continue
        fill, stroke = PAL[kind]
        p.append(f'<rect x="{x}" y="{y}" width="{w}" height="{h}" rx="5" fill="{fill}" '
                 f'stroke="{stroke}" stroke-width="1.5"/>')
        p.append(f'<text x="{x+w/2}" y="{y+h/2-2}" text-anchor="middle" font-family="monospace" '
                 f'font-size="12.5" font-weight="bold" fill="{INK}">{escape(label)}</text>')
        p.append(f'<text x="{x+w/2}" y="{y+h/2+14}" text-anchor="middle" '
                 f'font-family="sans-serif" font-size="9.5" fill="{stroke}">{escape(sub)}</text>')

    for _src, _dst, colour, pts in EDGES:
        d = " ".join(f"{round(x,1)},{round(y,1)}" for x, y in trim(pts))
        p.append(f'<polyline points="{d}" fill="none" stroke="{colour}" stroke-width="1.7" '
                 f'stroke-linejoin="round" marker-end="url(#h{colour[1:]})"/>')

    for x, y, colour, text, anchor in NOTES:
        p.append(f'<text x="{x}" y="{y}" text-anchor="{anchor}" font-family="sans-serif" '
                 f'font-size="10.5" fill="{colour}">{escape(text)}</text>')

    p.append(f'<text x="24" y="874" font-family="sans-serif" font-size="10.5" fill="#5A6B7D">'
             f'instruction_memory is addressed with PC_next, one cycle early, so the BSRAM&#8217;s '
             f'1-cycle latency costs the pipeline nothing.   The load result skips MEM/WB '
             f'entirely: data_memory&#8217;s own output register is that stage.</text>')
    p.append('</svg>')
    return "\n".join(p)


def build_mxfile():
    cells = ['<mxCell id="0"/>', '<mxCell id="1" parent="0"/>']
    for bid in BOX:
        _, x, y, w, h, kind, label, sub = BOX[bid]
        if kind == "bar":
            style = (f"rounded=0;whiteSpace=wrap;html=1;fillColor={BAR_FILL};"
                     f"strokeColor={BAR_STROKE};fontColor=#FFFFFF;fontSize=11;"
                     f"horizontal=0;fontStyle=1;")
            val = label
        else:
            fill, stroke = PAL[kind]
            style = (f"rounded=1;arcSize=10;whiteSpace=wrap;html=1;fillColor={fill};"
                     f"strokeColor={stroke};fontColor={INK};fontSize=11;fontFamily=Courier New;")
            val = label + ("&#10;" + sub if sub else "")
        cells.append(f'<mxCell id="{bid}" value="{val}" style="{style}" vertex="1" parent="1">'
                     f'<mxGeometry x="{x}" y="{y}" width="{w}" height="{h}" as="geometry"/>'
                     f'</mxCell>')
    for i, (src, dst, colour, _pts) in enumerate(EDGES):
        style = (f"edgeStyle=orthogonalEdgeStyle;rounded=1;html=1;strokeColor={colour};"
                 f"strokeWidth=1.7;endArrow=block;endFill=1;")
        cells.append(f'<mxCell id="e{i}" style="{style}" edge="1" parent="1" '
                     f'source="{src}" target="{dst}">'
                     f'<mxGeometry relative="1" as="geometry"/></mxCell>')
    return ('<mxfile host="app.diagrams.net"><diagram name="datapath">'
            f'<mxGraphModel dx="1400" dy="900" grid="0" gridSize="10" guides="1" tooltips="1" '
            f'connect="1" arrows="1" fold="1" page="1" pageScale="1" pageWidth="{W}" '
            f'pageHeight="{H}" math="0" shadow="0">'
            '<root>' + "".join(cells) + '</root></mxGraphModel></diagram></mxfile>')


if __name__ == "__main__":
    problems = check_collisions()
    if problems:
        print(f"{len(problems)} routing collision(s):", file=sys.stderr)
        for line in problems:
            print("  " + line, file=sys.stderr)
        sys.exit(1)
    sys.stdout.write(build_svg().replace("@@CONTENT@@",
                                         escape(build_mxfile(), {'"': "&quot;"})) + "\n")
