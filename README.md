# Path Tracer Benchmark Dashboard

## Summary Results
| Scene | Variant | Final RMSE | Date |
|---|---|---|---|
| default | **st** | 2.11070 | 3.58s | 2026-02-24T17:55:17.079280 |
| default | **mt** | 2.11070 | 1.24s | 2026-02-24T17:55:17.079280 |
| test | **mt** | 2.11070 | 1.21s | 2026-02-24T17:55:17.079280 |
| test | **st** | 2.11070 | 3.58s | 2026-02-24T17:55:17.079280 |


## RMSE Trend
![RMSE Trend](plots/history_score_trend.png)

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
### Scene: test

| Type | mt | st |
| :---: | :---: | :---: |
| **Render** | ![ mt ](renderings/latest-render_test_mt.png) | ![ st ](renderings/latest-render_test_st.png) |
| **Diff** | ![ diff mt ](renderings/latest-diff_render_test_mt.png) | ![ diff st ](renderings/latest-diff_render_test_st.png) |

### Convergence: test

![ test convergence ](plots/convergence_test.png)

---


---
*last updated: 2026-02-24T17:55:17.079280 (commit: d2ee357b)*
