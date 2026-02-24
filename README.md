# Path Tracer Benchmark Dashboard

## Summary Results
| Scene | Variant | RelMSE Score | Render Time | Iterations | Date |
|---|---|---|---|---|---|
| default | **st** | 0.87420 | 21.68s | 10 | 2026-02-24 22:19 |
| default | **mt** | 0.28260 | 70.45s | 100 | 2026-02-24 22:19 |
| test | **mt** | 0.28260 | 70.61s | 100 | 2026-02-24 22:19 |
| test | **st** | 0.87420 | 21.70s | 10 | 2026-02-24 22:19 |


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
### Scene: test

| Type | mt | st |
| :---: | :---: | :---: |
| **Render** | ![ mt ](renderings/latest-render_test_mt.png) | ![ st ](renderings/latest-render_test_st.png) |
| **Diff** | ![ diff mt ](renderings/latest-diff_render_test_mt.png) | ![ diff st ](renderings/latest-diff_render_test_st.png) |

### Convergence: test

![ test convergence ](plots/convergence_test.png)

---


---
*last updated: 2026-02-24 22:19 (commit: 53316495)*
