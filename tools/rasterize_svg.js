#!/usr/bin/env node
const fs = require("fs");
const path = require("path");
const svgPath = process.argv[2];
const pngPath = process.argv[3];
const width = parseInt(process.argv[4] || "256", 10);
const roots = [
  path.join("/tmp", "nrl-resvg", "node_modules", "@resvg", "resvg-js"),
  path.join(__dirname, ".resvg", "node_modules", "@resvg", "resvg-js"),
  "@resvg/resvg-js"
];
let Resvg;
for (const mod of roots) {
  try {
    Resvg = require(mod).Resvg;
    break;
  } catch (err) {
    Resvg = null;
  }
}
if (!Resvg) {
  process.stderr.write("resvg-js not found\n");
  process.exit(1);
}
const svg = fs.readFileSync(svgPath);
const resvg = new Resvg(svg, {
  fitTo: { mode: "width", value: width },
  background: "rgba(0,0,0,0)"
});
fs.writeFileSync(pngPath, resvg.render().asPng());
