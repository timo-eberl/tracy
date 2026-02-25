# Path Tracer Benchmark Dashboard

## Summary Results
| Scene | Variant | RelMSE Score | Render Time | Iterations | Date |
|---|---|---|---|---|---|
| caustics | **rr** | 1.42000 | 1.84s | 40 | 2026-02-25 19:23 |
| cornell | **std** | 0.63910 | 20.06s | 30 | 2026-02-25 19:23 |


## RelMSE Trend
![RelMSE Trend](plots/history_score_trend.png)

## Time Taken Trend
![Time Taken Trend](plots/history_time_trend.png)

## Render Gallery & Convergence
### Scene: caustics

| Type | rr | Reference |
| :---: | :---: | :---: |
| **Render** | ![ rr ](renderings/latest-render_caustics_rr.png) | ![ Reference](renderings/reference_caustics.png) |
| **Diff** | ![ diff rr ](renderings/latest-diff_render_caustics_rr.png) | |

### Convergence: caustics

![ caustics convergence ](plots/convergence_caustics.png)

---
### Scene: cornell

| Type | std | Reference |
| :---: | :---: | :---: |
| **Render** | ![ std ](renderings/latest-render_cornell_std.png) | ![ Reference](renderings/reference_cornell.png) |
| **Diff** | ![ diff std ](renderings/latest-diff_render_cornell_std.png) | |

### Convergence: cornell

![ cornell convergence ](plots/convergence_cornell.png)

---


---
*last updated: 2026-02-25 19:23 (commit: 98391388)*
