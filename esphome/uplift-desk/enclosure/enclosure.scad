// Uplift desk ESP32 + RJ45 breakout enclosure
// Units: mm. Render with: part = "base" | "lid" | "both" | "preview"

part = "preview";

// --- Print tuning ---------------------------------------------------------
fit   = 0.5;   // total clearance added to each board dimension
wall  = 2.0;
floor_t = 2.0;
lid_t = 2.0;
lid_lip_h = 3.0;
lid_lip_t = 1.2;
lid_fit = 0.25; // gap between lid lip and inner wall, per side

// --- RJ45 breakout board (docs/rj45-breakout-board.md) --------------------
rj_len   = 57.48;  // along the port axis (ports on both ends)
rj_wid   = 28.03;
rj_pcb_t = 1.63;
rj_riser = 3.5;    // lift above floor to clear 2-3mm of solder on the bottom

jack_w = 16.3;
jack_h = 13.34;
jack_margin = 0.6; // extra opening around the jack, per side

// Corner pads sit under the mounting-hole corners
pad = 7;
guide_t = 1.2;     // locating tabs along the inner (ESP-side) edge
guide_h = 3;       // how far tabs rise above the PCB bottom

// --- ESP32 DevKit V1 (USB-C) ----------------------------------------------
esp_len = 51;
esp_wid = 28;
esp_guides = true;  // low corner brackets so it doesn't slide around
esp_guide_h = 3;
esp_guide_leg = 6;

// --- Layout ---------------------------------------------------------------
gap = 8;           // open space between boards for wiring
inner_h = 24;      // tallest stack: ESP32 + headers + wire is ~20mm

inner_x = (rj_wid + fit) + gap + (esp_wid + fit);
inner_y = max(rj_len, esp_len) + fit;
outer_x = inner_x + 2 * wall;
outer_y = inner_y + 2 * wall;
outer_h = floor_t + inner_h;

// Board origins (inner-cavity coordinates offset by wall)
rj_x0  = wall + fit / 2;
rj_y0  = wall + (inner_y - rj_len) / 2;
esp_x0 = wall + inner_x - esp_wid - fit / 2;
esp_y0 = wall + (inner_y - esp_len) / 2;

pcb_bottom_z = floor_t + rj_riser;
jack_z0 = pcb_bottom_z + rj_pcb_t;

corner_r = 2;
$fn = 32;

module rounded_box(x, y, h, r) {
    hull() for (i = [r, x - r], j = [r, y - r])
        translate([i, j, 0]) cylinder(r = r, h = h);
}

module jack_cutout() {
    cw = jack_w + 2 * jack_margin;
    ch = jack_h + 2 * jack_margin;
    cx = rj_x0 + rj_wid / 2 - cw / 2;
    for (y = [-1, outer_y - wall - 1])
        translate([cx, y, jack_z0 - jack_margin]) cube([cw, wall + 2, ch]);
}

module rj_supports() {
    // Corner pads
    for (i = [0, 1], j = [0, 1])
        translate([rj_x0 + i * (rj_wid - pad), rj_y0 + j * (rj_len - pad), floor_t - 0.01])
            cube([pad, pad, rj_riser + 0.01]);
    // Tabs on the ESP-side edge stop the board sliding sideways
    for (j = [0, 1])
        translate([rj_x0 + rj_wid + fit / 2, rj_y0 + j * (rj_len - pad), floor_t - 0.01])
            cube([guide_t, pad, rj_riser + guide_h + 0.01]);
}

module esp_supports() {
    l = esp_guide_leg;
    h = esp_guide_h + 0.01;
    o = fit / 2 + guide_t; // guide sits just outside the board outline
    x0 = esp_x0 - o;
    y0 = esp_y0 - o;
    x1 = esp_x0 + esp_wid + o;
    y1 = esp_y0 + esp_len + o;
    // [corner x, corner y, x direction into board, y direction into board]
    for (c = [[x0, y0, 1, 1], [x1, y0, -1, 1], [x0, y1, 1, -1], [x1, y1, -1, -1]]) {
        translate([c[2] > 0 ? c[0] : c[0] - l, c[3] > 0 ? c[1] : c[1] - guide_t, floor_t - 0.01])
            cube([l, guide_t, h]);
        translate([c[2] > 0 ? c[0] : c[0] - guide_t, c[3] > 0 ? c[1] : c[1] - l, floor_t - 0.01])
            cube([guide_t, l, h]);
    }
}

module base() {
    difference() {
        rounded_box(outer_x, outer_y, outer_h, corner_r);
        translate([wall, wall, floor_t]) cube([inner_x, inner_y, inner_h + 1]);
        jack_cutout();
    }
    intersection() {
        // Keep supports inside the cavity
        translate([wall, wall, 0]) cube([inner_x, inner_y, outer_h]);
        union() {
            rj_supports();
            if (esp_guides) esp_supports();
        }
    }
}

module lid() {
    rounded_box(outer_x, outer_y, lid_t, corner_r);
    lx = inner_x - 2 * lid_fit;
    ly = inner_y - 2 * lid_fit;
    translate([wall + lid_fit, wall + lid_fit, lid_t - 0.01])
        difference() {
            cube([lx, ly, lid_lip_h]);
            translate([lid_lip_t, lid_lip_t, -1])
                cube([lx - 2 * lid_lip_t, ly - 2 * lid_lip_t, lid_lip_h + 2]);
        }
}

// Ghost boards for visual checking only
module ghosts() {
    %translate([rj_x0, rj_y0, pcb_bottom_z]) {
        cube([rj_wid, rj_len, rj_pcb_t]);
        for (y = [0, rj_len - 15.55])
            translate([rj_wid / 2 - jack_w / 2, y, rj_pcb_t]) cube([jack_w, 15.55, jack_h]);
    }
    %translate([esp_x0, esp_y0, floor_t]) cube([esp_wid, esp_len, 20]);
}

if (part == "base") base();
else if (part == "lid") lid();
else if (part == "both") {
    base();
    translate([outer_x + 10, 0, 0]) lid();
}
else { // preview
    base();
    ghosts();
}
