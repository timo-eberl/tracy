# Path Tracer Benchmark Dashboard

## Summary Results
| Scene | Variant | RelMSE Score | Render Time | Iterations | Date |
|---|---|---|---|---|---|
| caustics | **rr** | 0.58730 | 6.55s | 200 | 2026-03-04 21:32 |
| caustics | **std** | 0.61090 | 6.54s | 200 | 2026-03-04 21:32 |
| cornell-v2 | **rr** | 0.16730 | 34.22s | 150 | 2026-03-04 21:32 |
| cornell-v2 | **std** | 0.16330 | 43.00s | 150 | 2026-03-04 21:32 |
| glass-sphere | **rr** | 0.06020 | 1.77s | 1000 | 2026-03-04 21:32 |


## RelMSE Trend
![RelMSE Trend](plots/history_score_trend.png)

## Runtime Trend
![Runtime Trend](plots/history_time_trend.png)

## Render Gallery & Convergence
### Scene: caustics

| Type | rr | std | Reference |
| :---: | :---: | :---: | :---: |
| **Render** | ![ rr ](renderings/latest-render_caustics_rr.png) | ![ std ](renderings/latest-render_caustics_std.png) | ![ Reference](renderings/reference_caustics.png) |
| **Diff** | ![ diff rr ](renderings/latest-diff_render_caustics_rr.png) | ![ diff std ](renderings/latest-diff_render_caustics_std.png) | |

### Convergence: caustics

![ caustics convergence ](plots/convergence_caustics.png)

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
*last updated: 2026-03-04 21:32 (commit: 3c4c4dde)*
