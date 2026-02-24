# Path Tracer Benchmark Dashboard

## Summary Results
| Scene | Variant | Final RMSE | Date |
|---|---|---|---|
| default | **st** | 2.11070 | 3.59s | 2026-02-24T17:13:25.939766 |
| default | **mt** | 2.11070 | 1.21s | 2026-02-24T17:13:25.939766 |
| test | **mt** | 2.11070 | 1.21s | 2026-02-24T17:13:25.939766 |
| test | **st** | 2.11070 | 3.60s | 2026-02-24T17:13:25.939766 |


## RMSE Trend
![RMSE Trend](renderings/history_score_trend.png)

## Time Taken Trend
![Time Taken Trend](renderings/history_time_trend.png)

## Render Gallery & Convergence
### Scene: default

| Type | mt | st |
| :---: | :---: | :---: |
| **Render** | ![ mt ](renderings/latest-render_default_mt.png) | ![ st ](renderings/latest-render_default_st.png) |
| **Diff** | ![ diff mt ](renderings/diff_render_default_mt.png) | ![ diff st ](renderings/diff_render_default_st.png) |

**Convergence for default:**

![ default convergence ](renderings/convergence_default.png)

---
### Scene: test

| Type | mt | st |
| :---: | :---: | :---: |
| **Render** | ![ mt ](renderings/latest-render_test_mt.png) | ![ st ](renderings/latest-render_test_st.png) |
| **Diff** | ![ diff mt ](renderings/diff_render_test_mt.png) | ![ diff st ](renderings/diff_render_test_st.png) |

**Convergence for test:**

![ test convergence ](renderings/convergence_test.png)

---


---
*last updated: 2026-02-24T17:13:25.939766 (commit: 1ca07ab0)*
