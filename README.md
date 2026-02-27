# Path Tracer Benchmark Dashboard

## Summary Results
| Scene | Variant | RelMSE Score | Render Time | Iterations | Date |
|---|---|---|---|---|---|
| caustics | **rr** | 0.65010 | 8.85s | 200 | 2026-02-27 19:25 |
| caustics | **std** | 0.63150 | 8.92s | 200 | 2026-02-27 19:25 |
| cornell | **std** | 0.36620 | 42.92s | 60 | 2026-02-27 19:25 |
| cornell | **rr** | 0.38780 | 36.43s | 60 | 2026-02-27 19:25 |


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


---
*last updated: 2026-02-27 19:25 (commit: f0f80664)*
