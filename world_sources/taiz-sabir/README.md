# Taiz to Mount Sabir road centerline

This environment-art source follows 5,095.2 m of mapped streets between
central Taiz and a switchback on Mount Sabir Road. The line has 152 vertices
from ten OpenStreetMap ways. Its sampled surfaces start at 1,355.6 m and end
at 1,841.4 m. It is intended for the first scene blockout. Author the road
grade, camber, junctions and overlook before using it in the driving runtime.

`centerline.geojson` stores longitude, latitude and sampled surface height in
metres. The DEM represents a surface, including objects and vegetation, so
the samples guide broad contours. Posted limits, turns and lane assignments
require a separate authoring and validation pass.

## Reproduce

Use GDAL and Python 3. Fetch the source inputs and verify their checksums.

| Input | SHA-256 |
| --- | --- |
| [Geofabrik Yemen PBF](https://download.geofabrik.de/asia/yemen.html), snapshot downloaded 2026-11-24 | `8ce0919e8076a1f088ce2bb34f8f95bfaa1e36bb45ff34238eb5283c6d995f32` |
| [Copernicus GLO-30 N13 E044](https://copernicus-dem-30m.s3.amazonaws.com/Copernicus_DSM_COG_10_N13_00_E044_00_DEM/Copernicus_DSM_COG_10_N13_00_E044_00_DEM.tif) | `6930488d8beb988d1eee1ed33ddd5c6641974783e060a5379cc01760629825f9` |

```sh
ogr2ogr -f GeoJSON -spat 43.995 13.515 44.075 13.617 \
  -where 'highway IS NOT NULL' taiz-roads.json yemen.osm.pbf lines
python3 tools/survey_taiz_sabir.py taiz-roads.json taiz-glo30.tif \
  world_sources/taiz-sabir/centerline.geojson
```

## Source credits

Road geometry © OpenStreetMap contributors. The road centerline data
are available under the [Open Database License 1.0](https://opendatacommons.org/licenses/odbl/1-0/).
[OpenStreetMap attribution and terms](https://www.openstreetmap.org/copyright).

Elevation produced using Copernicus WorldDEM-30 © DLR e.V. 2010-2014 and
© Airbus Defence and Space GmbH 2014-2018 provided under COPERNICUS by the
European Union and ESA; all rights reserved. The organisations in charge of
the Copernicus programme by law or by delegation do not incur any liability
for any use of the Copernicus WorldDEM-30.
[Copernicus DEM licence](https://documentation.dataspace.copernicus.eu/APIs/SentinelHub/Data/DEM/resources/license/License-COPDEM-30.pdf).
