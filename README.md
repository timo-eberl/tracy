# Path Tracer Benchmark Dashboard

## Summary Results
| Scene | Variant | RelMSE Score | Render Time | Iterations | Date |
|---|---|---|---|---|---|
| caustics | **rr** | 0.58730 | 6.53s | 200 | 2026-03-04 16:44 |
| caustics | **std** | 0.61090 | 6.53s | 200 | 2026-03-04 16:44 |
| cornell | **std** | 0.36090 | 29.77s | 60 | 2026-03-04 16:44 |
| cornell | **rr** | 0.40520 | 28.22s | 60 | 2026-03-04 16:44 |
| glass_sphere | **sphere** | 0.06020 | 1.77s | 1000 | 2026-03-04 16:44 |


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
### Scene: cornell

| Type | rr | std | Reference |
| :---: | :---: | :---: | :---: |
| **Render** | ![ rr ](renderings/latest-render_cornell_rr.png) | ![ std ](renderings/latest-render_cornell_std.png) | ![ Reference](renderings/reference_cornell.png) |
| **Diff** | ![ diff rr ](renderings/latest-diff_render_cornell_rr.png) | ![ diff std ](renderings/latest-diff_render_cornell_std.png) | |

### Convergence: cornell

![ cornell convergence ](plots/convergence_cornell.png)

---
### Scene: glass_sphere

| Type | sphere | Reference |
| :---: | :---: | :---: |
| **Render** | ![ sphere ](renderings/latest-render_glass_sphere_sphere.png) | ![ Reference](renderings/reference_glass_sphere.png) |
| **Diff** | ![ diff sphere ](renderings/latest-diff_render_glass_sphere_sphere.png) | |

### Convergence: glass_sphere

![ glass_sphere convergence ](plots/convergence_glass_sphere.png)

---


---
*last updated: 2026-03-04 16:44 (commit: 1c5860d6)*
