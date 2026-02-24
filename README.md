# Path Tracer Benchmark Dashboard

## Summary Results
| Scene | Variant | RelMSE Score | Render Time | Iterations | Date |
|---|---|---|---|---|---|
| example | **3** | nan | 38.72s | 200 | 2026-02-24 23:52 |
| example | **1** | 0.28260 | 70.40s | 100 | 2026-02-24 23:52 |
| example | **2** | 0.30170 | 60.63s | 100 | 2026-02-24 23:52 |
| example | **4** | nan | 38.52s | 200 | 2026-02-24 23:52 |


## RelMSE Trend
![RelMSE Trend](plots/history_score_trend.png)

## Time Taken Trend
![Time Taken Trend](plots/history_time_trend.png)

## Render Gallery & Convergence
### Scene: example

| Type | 1 | 2 | 3 | 4 |
| :---: | :---: | :---: | :---: | :---: |
| **Render** | ![ 1 ](renderings/latest-render_example_1.png) | ![ 2 ](renderings/latest-render_example_2.png) | ![ 3 ](renderings/latest-render_example_3.png) | ![ 4 ](renderings/latest-render_example_4.png) |
| **Diff** | ![ diff 1 ](renderings/latest-diff_render_example_1.png) | ![ diff 2 ](renderings/latest-diff_render_example_2.png) | ![ diff 3 ](renderings/latest-diff_render_example_3.png) | ![ diff 4 ](renderings/latest-diff_render_example_4.png) |

### Convergence: example

![ example convergence ](plots/convergence_example.png)

---


---
*last updated: 2026-02-24 23:52 (commit: ebcdffb7)*
