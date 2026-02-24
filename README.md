# Path Tracer Benchmark Dashboard

## Summary Results
| Scene | Variant | RelMSE Score | Render Time | Iterations | Date |
|---|---|---|---|
| default | **st** | 2.11070 | 3.18s | 2 | 2026-02-24T20:58:15.716278 |
| default | **mt** | 0.94910 | 6.02s | 10 | 2026-02-24T20:58:15.716278 |
| test | **mt** | 0.94910 | 5.94s | 10 | 2026-02-24T20:58:15.716278 |
| test | **st** | 2.11070 | 3.20s | 2 | 2026-02-24T20:58:15.716278 |


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
*last updated: 2026-02-24T20:58:15.716278 (commit: b65bc6ae)*
