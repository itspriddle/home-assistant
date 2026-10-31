# Visualization Examples

Here are the configurations to visualize your thermostat usage using `mini-graph-card`.

## 1. Daily Heating Usage (Bar Chart)
This shows how many hours the heating ran each day for the last 7 days.

```yaml
type: custom:mini-graph-card
name: Heating History (Last 7 Days)
icon: mdi:fire
entities:
  - entity: sensor.thermostat_heating_today
    name: Heating
    color: "#ff9800"
show:
  graph: bar
  labels: true
group_by: date
aggregate_func: max
hours_to_show: 168
bar_spacing: 10
```

## 2. Daily Cooling Usage (Bar Chart)
Same as above, but for cooling.

```yaml
type: custom:mini-graph-card
name: Cooling History (Last 7 Days)
icon: mdi:snowflake
entities:
  - entity: sensor.thermostat_cooling_today
    name: Cooling
    color: "#2196f3"
show:
  graph: bar
  labels: true
group_by: date
aggregate_func: max
hours_to_show: 168
bar_spacing: 10
```

## 3. Timeline (State History)
To see *when* it turned on/off throughout the day, the standard History Graph is actually best.

```yaml
type: history-graph
entities:
  - entity: climate.thermostat
  - entity: binary_sensor.thermostat_heating
  - entity: binary_sensor.thermostat_cooling
hours_to_show: 24
refresh_interval: 0
```
