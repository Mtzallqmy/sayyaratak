#!/usr/bin/env python3
"""Derive the Taiz–Sabir road centerline for the environment-art blockout."""

import argparse
from collections import defaultdict
import heapq
import json
import math
from pathlib import Path
import subprocess

START = (44.0213901, 13.5753839)
FINISH = (44.0188150, 13.5563387)
MOTOR_ROADS = {
    "primary", "primary_link", "secondary", "secondary_link", "tertiary",
    "tertiary_link", "unclassified", "residential", "service", "living_street",
}


def distance(a, b):
    longitude = math.radians(b[0] - a[0]) * math.cos(math.radians((a[1] + b[1]) / 2))
    latitude = math.radians(b[1] - a[1])
    return math.hypot(longitude, latitude) * 6_371_008.8


def route_through(roads):
    graph = defaultdict(list)
    for feature in roads["features"]:
        tags = feature["properties"]
        geometry = feature["geometry"]
        if tags.get("highway") not in MOTOR_ROADS or geometry["type"] != "LineString":
            continue
        extra = tags.get("other_tags") or ""
        if any(f'"{key}"=>"{value}"' in extra for key, value in (
                ("access", "private"), ("access", "no"), ("motor_vehicle", "no"),
                ("motorcar", "no"), ("motor_vehicle", "private"))):
            continue
        way = int(tags["osm_id"])
        points = [tuple(point) for point in geometry["coordinates"]]
        reverse_only = '"oneway"=>"-1"' in extra
        forward_only = any(rule in extra for rule in (
            '"oneway"=>"yes"', '"oneway"=>"1"', '"junction"=>"roundabout"',
        ))
        for a, b in zip(points, points[1:]):
            if a == b:
                continue
            length = distance(a, b)
            if not reverse_only:
                graph[a].append((b, length, way))
            if not forward_only or reverse_only:
                graph[b].append((a, length, way))

    queue = [(0.0, START)]
    costs = {START: 0.0}
    previous = {}
    while queue:
        length, point = heapq.heappop(queue)
        if length != costs[point]:
            continue
        if point == FINISH:
            break
        for end, edge_length, way in graph[point]:
            new_cost = length + edge_length
            if new_cost < costs.get(end, math.inf):
                costs[end] = new_cost
                previous[end] = (point, way)
                heapq.heappush(queue, (new_cost, end))

    if FINISH not in costs or not 4_000 <= costs[FINISH] <= 6_000:
        raise SystemExit("The source snapshot has no 4–6 km route between the recorded vertices.")
    centerline = [FINISH]
    ways = []
    while centerline[-1] != START:
        previous_point, way = previous[centerline[-1]]
        centerline.append(previous_point)
        ways.append(way)
    return centerline[::-1], list(dict.fromkeys(ways[::-1])), costs[FINISH]


def elevation_profile(dem, centerline):
    coords = "".join(f"{longitude:.7f} {latitude:.7f}\n" for longitude, latitude in centerline)
    result = subprocess.run(
        ["gdallocationinfo", "-wgs84", "-valonly", str(dem)], input=coords,
        capture_output=True, check=True, text=True,
    )
    heights = [float(line) for line in result.stdout.splitlines() if line.strip()]
    if len(heights) != len(centerline) or any(not 800 < z < 3300 for z in heights):
        raise SystemExit("The Copernicus tile did not provide a valid height for every vertex.")
    return heights


def main():
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument("roads", type=Path, help="OSM lines exported via the GDAL OSM driver")
    parser.add_argument("dem", type=Path, help="Copernicus GLO-30 N13 E044 GeoTIFF")
    parser.add_argument("output", type=Path)
    args = parser.parse_args()

    centerline, ways, meters = route_through(json.loads(args.roads.read_text(encoding="utf-8")))
    elevations = elevation_profile(args.dem, centerline)
    result = {
        "type": "Feature",
        "properties": {
            "name": "تعز — طريق جبل صبر",
            "length_m": round(meters, 1),
            "source_osm_ways": ways,
            "start_elevation_m": round(elevations[0], 1),
            "finish_elevation_m": round(elevations[-1], 1),
            "height_source": "Copernicus GLO-30 DSM; sampled surface, requiring road grade adjustment",
        },
        "geometry": {
            "type": "LineString",
            "coordinates": [[x, y, round(z, 1)] for (x, y), z in zip(centerline, elevations)],
        },
    }
    args.output.write_text(json.dumps(result, ensure_ascii=False, separators=(",", ":")) + "\n", encoding="utf-8")
    print(f"{meters:.1f} m, {len(centerline)} vertices, {len(ways)} OSM ways, {elevations[0]:.1f} to {elevations[-1]:.1f} m")


if __name__ == "__main__":
    main()
