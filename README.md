# Benchmark
## Summary
| Mode | Final RMSE | Total Time | Steps |
|---|---|---|---|
| **MT** | 0.6796 | 67.37s | 10 |
| **ST** | 0.6796 | 203.28s | 10 |

## Trend
```mermaid
xychart-beta
    title "Trend"
    x-axis ["b.66", "b.67"]
    y-axis "RMSE" 0 --> 1.7257
    line [1.4381, 0.6796]
    line [1.4381, 0.6796]
```
## Gallery
| MT | ST |
| :---: | :---: |
| ![ mt ](renderings/latest-mt.png) | ![ st ](renderings/latest-st.png) |
## Convergence
```mermaid
xychart-beta
    title "Convergence"
    x-axis ["0.0s", "22.59s", "45.17s", "67.76s", "90.34s", "112.93s", "135.52s", "158.1s", "180.69s", "203.28s"]
    y-axis "RMSE" 0 --> 2.2307
    line [2.0279, 1.185, 0.8561, 0.6796, 0.6796, 0.6796, 0.6796, 0.6796, 0.6796, 0.6796]
    line [2.0279, 2.0279, 1.4381, 1.185, 1.0336, 0.9306, 0.8561, 0.7964, 0.7495, 0.6796]
```