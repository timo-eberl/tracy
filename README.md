# Path Tracer Benchmark Dashboard

## Summary Results
| Scene | Variant | RelMSE Score | Render Time | Iterations | Date |
|---|---|---|---|---|---|
| caustics | **rr** | 0.65010 | 8.51s | 200 | 2026-02-27 20:52 |
| caustics | **std** | 0.63150 | 8.68s | 200 | 2026-02-27 20:52 |
| cornell | **std** | 0.36620 | 40.99s | 60 | 2026-02-27 20:52 |
| cornell | **rr** | 0.38780 | 35.73s | 60 | 2026-02-27 20:52 |


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
*last updated: 2026-02-27 20:52 (commit: e6a29507)*
