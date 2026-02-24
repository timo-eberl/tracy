# Path Tracer Benchmark Dashboard

## Summary Results
| Scene | Variant | RelMSE Score | Render Time | Iterations | Date |
|---|---|---|---|---|---|
| default | **st** | 0.87420 | 21.69s | 10 | 2026-02-24 22:26 |
| default | **mt** | 0.28260 | 70.47s | 100 | 2026-02-24 22:26 |
| example | **3** | 421.84940 | 56.25s | 50 | 2026-02-24 22:48 |
| example | **1** | 0.28260 | 70.47s | 100 | 2026-02-24 22:48 |
| example | **2** | 0.61930 | 42.53s | 20 | 2026-02-24 22:48 |
| test | **mt** | 0.28260 | 70.51s | 100 | 2026-02-24 22:26 |
| test | **st** | 0.87420 | 21.69s | 10 | 2026-02-24 22:26 |


## RelMSE Trend
![RelMSE Trend](plots/history_score_trend.png)

## Time Taken Trend
![Time Taken Trend](plots/history_time_trend.png)

## Render Gallery & Convergence
### Scene: default

| Type | mt | st |
| :---: | :---: | :---: |
| **Render** | ![ mt ](renderings/latest-render_default_mt.png) | ![ st ](renderings/latest-render_default_st.png) |
| **Diff** | ![ diff mt ](renderings/latest-diff_render_default_mt.png) | ![ diff st ](renderings/latest-diff_render_default_st.png) |

### Convergence: default

![ default convergence ](plots/convergence_default.png)

---
### Scene: example

| Type | 1 | 2 | 3 |
| :---: | :---: | :---: | :---: |
| **Render** | ![ 1 ](renderings/latest-render_example_1.png) | ![ 2 ](renderings/latest-render_example_2.png) | ![ 3 ](renderings/latest-render_example_3.png) |
| **Diff** | ![ diff 1 ](renderings/latest-diff_render_example_1.png) | ![ diff 2 ](renderings/latest-diff_render_example_2.png) | ![ diff 3 ](renderings/latest-diff_render_example_3.png) |

### Convergence: example

![ example convergence ](plots/convergence_example.png)

---
### Scene: test

| Type | mt | st |
| :---: | :---: | :---: |
| **Render** | ![ mt ](renderings/latest-render_test_mt.png) | ![ st ](renderings/latest-render_test_st.png) |
| **Diff** | ![ diff mt ](renderings/latest-diff_render_test_mt.png) | ![ diff st ](renderings/latest-diff_render_test_st.png) |

### Convergence: test

![ test convergence ](plots/convergence_test.png)

---


---
*last updated: 2026-02-24 22:48 (commit: 6ec9a4a5)*
