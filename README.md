# Benchmark
## Summary
| Mode | Final RMSE | Total Time | Steps |
|---|---|---|---|
| **MT** | 1.4381 | 13.56s | 2 |
| **ST** | 1.4381 | 40.57s | 2 |

## Trend
```mermaid
xychart-beta
    title "Trend"
    x-axis ["b.66"]
    y-axis "RMSE" 0 --> 1.7257
    line [1.4381]
    line [1.4381]
```
## Gallery
| MT | ST |
| :---: | :---: |
| ![ mt ](renderings/latest-mt.png) | ![ st ](renderings/latest-st.png) |
## Convergence
```mermaid
xychart-beta
    title "Convergence"
    x-axis ["0.0s", "4.51s", "9.02s", "13.52s", "18.03s", "22.54s", "27.05s", "31.55s", "36.06s", "40.57s"]
    y-axis "RMSE" 0 --> 2.2307
    line [2.0279, 2.0279, 2.0279, 2.0279, 1.4381, 1.4381, 1.4381, 1.4381, 1.4381, 1.4381]
    line [2.0279, 2.0279, 2.0279, 2.0279, 2.0279, 2.0279, 2.0279, 2.0279, 2.0279, 1.4381]
```