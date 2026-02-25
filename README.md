# Path Tracer Benchmark Dashboard

## Summary Results
| Scene | Variant | RelMSE Score | Render Time | Iterations | Date |
|---|---|---|---|---|---|
| cornell | **std** | 3.30870 | 0.71s | 1 | 2026-02-25 16:32 |
| example1 | **std** | 1.24670 | 3.53s | 5 | 2026-02-25 13:00 |
| example2 | **rr** | 1.35390 | 3.11s | 5 | 2026-02-25 13:00 |


## RelMSE Trend
![RelMSE Trend](plots/history_score_trend.png)

## Time Taken Trend
![Time Taken Trend](plots/history_time_trend.png)

## Render Gallery & Convergence
### Scene: cornell

| Type | std | Reference |
| :---: | :---: | :---: |
| **Render** | ![ std ](renderings/latest-render_cornell_std.png) | ![ Reference](renderings/reference_cornell.png) |
| **Diff** | ![ diff std ](renderings/latest-diff_render_cornell_std.png) | |

### Convergence: cornell

![ cornell convergence ](plots/convergence_cornell.png)

---
### Scene: example1

| Type | std | Reference |
| :---: | :---: | :---: |
| **Render** | ![ std ](renderings/latest-render_example1_std.png) | ![ Reference](renderings/reference_example1.png) |
| **Diff** | ![ diff std ](renderings/latest-diff_render_example1_std.png) | |

### Convergence: example1

![ example1 convergence ](plots/convergence_example1.png)

---
### Scene: example2

| Type | rr | Reference |
| :---: | :---: | :---: |
| **Render** | ![ rr ](renderings/latest-render_example2_rr.png) | ![ Reference](renderings/reference_example2.png) |
| **Diff** | ![ diff rr ](renderings/latest-diff_render_example2_rr.png) | |

### Convergence: example2

![ example2 convergence ](plots/convergence_example2.png)

---


---
*last updated: 2026-02-25 16:32 (commit: 8b0c75e4)*
