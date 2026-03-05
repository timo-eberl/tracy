# Path Tracer Benchmark Dashboard

## Summary Results
| Scene | Variant | RelMSE Score | Render Time | Iterations | Date |
|---|---|---|---|---|---|
| caustics-v2 | **rr** | 0.64120 | 18.97s | 400 | 2026-03-05 00:33 |
| caustics-v2 | **std** | 0.63970 | 19.10s | 400 | 2026-03-05 00:33 |
| cornell-v2 | **rr** | 0.16730 | 34.14s | 75 | 2026-03-05 00:33 |
| cornell-v2 | **std** | 0.16310 | 43.50s | 75 | 2026-03-05 00:33 |
| glass-sphere | **rr** | 0.02770 | 1.55s | 500 | 2026-03-05 00:33 |


## RelMSE Trend
![RelMSE Trend](plots/history_score_trend.png)

## Runtime Trend
![Runtime Trend](plots/history_time_trend.png)

## Render Gallery & Convergence
### Scene: caustics-v2

| Type | rr | std | Reference |
| :---: | :---: | :---: | :---: |
| **Render** | ![ rr ](renderings/latest-render_caustics-v2_rr.png) | ![ std ](renderings/latest-render_caustics-v2_std.png) | ![ Reference](renderings/reference_caustics-v2.png) |
| **Diff** | ![ diff rr ](renderings/latest-diff_render_caustics-v2_rr.png) | ![ diff std ](renderings/latest-diff_render_caustics-v2_std.png) | |

### Convergence: caustics-v2

![ caustics-v2 convergence ](plots/convergence_caustics-v2.png)

---
### Scene: cornell-v2

| Type | rr | std | Reference |
| :---: | :---: | :---: | :---: |
| **Render** | ![ rr ](renderings/latest-render_cornell-v2_rr.png) | ![ std ](renderings/latest-render_cornell-v2_std.png) | ![ Reference](renderings/reference_cornell-v2.png) |
| **Diff** | ![ diff rr ](renderings/latest-diff_render_cornell-v2_rr.png) | ![ diff std ](renderings/latest-diff_render_cornell-v2_std.png) | |

### Convergence: cornell-v2

![ cornell-v2 convergence ](plots/convergence_cornell-v2.png)

---
### Scene: glass-sphere

| Type | rr | Reference |
| :---: | :---: | :---: |
| **Render** | ![ rr ](renderings/latest-render_glass-sphere_rr.png) | ![ Reference](renderings/reference_glass-sphere.png) |
| **Diff** | ![ diff rr ](renderings/latest-diff_render_glass-sphere_rr.png) | |

### Convergence: glass-sphere

![ glass-sphere convergence ](plots/convergence_glass-sphere.png)

---


---
*last updated: 2026-03-05 00:33 (commit: 6e090cb1)*
