# Path Tracer Benchmark Dashboard

## Summary Results
| Scene | Variant | RelMSE Score | Render Time | Iterations | Date |
|---|---|---|---|---|---|
| SCENE:example1 | **std** | 1.24670 | 3.52s | 5 | 2026-02-25 12:44 |
| SCENE:example2 | **rr** | 1.35390 | 3.03s | 5 | 2026-02-25 12:44 |
| example1 | **std** | 1.24670 | 3.53s | 5 | 2026-02-25 12:51 |
| example2 | **rr** | 1.35390 | 3.03s | 5 | 2026-02-25 12:51 |


## RelMSE Trend
![RelMSE Trend](plots/history_score_trend.png)

## Time Taken Trend
![Time Taken Trend](plots/history_time_trend.png)

## Render Gallery & Convergence
### Scene: SCENE:example1

| Type | std | Reference |
| :---: | :---: | :---: |
| **Render** | ![ std ](renderings/latest-render_SCENE:example1_std.png) | ![ Reference](renderings/reference.png) |
| **Diff** | ![ diff std ](renderings/latest-diff_render_SCENE:example1_std.png) | |

### Convergence: SCENE:example1

![ SCENE:example1 convergence ](plots/convergence_SCENE:example1.png)

---
### Scene: SCENE:example2

| Type | rr | Reference |
| :---: | :---: | :---: |
| **Render** | ![ rr ](renderings/latest-render_SCENE:example2_rr.png) | ![ Reference](renderings/reference.png) |
| **Diff** | ![ diff rr ](renderings/latest-diff_render_SCENE:example2_rr.png) | |

### Convergence: SCENE:example2

![ SCENE:example2 convergence ](plots/convergence_SCENE:example2.png)

---
### Scene: example1

| Type | std | Reference |
| :---: | :---: | :---: |
| **Render** | ![ std ](renderings/latest-render_example1_std.png) | ![ Reference](renderings/reference.png) |
| **Diff** | ![ diff std ](renderings/latest-diff_render_example1_std.png) | |

### Convergence: example1

![ example1 convergence ](plots/convergence_example1.png)

---
### Scene: example2

| Type | rr | Reference |
| :---: | :---: | :---: |
| **Render** | ![ rr ](renderings/latest-render_example2_rr.png) | ![ Reference](renderings/reference.png) |
| **Diff** | ![ diff rr ](renderings/latest-diff_render_example2_rr.png) | |

### Convergence: example2

![ example2 convergence ](plots/convergence_example2.png)

---


---
*last updated: 2026-02-25 12:51 (commit: f02efd7d)*
