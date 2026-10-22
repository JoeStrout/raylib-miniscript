# quadtree plugin

A point quadtree for fast "what is near here?" queries. Points are stored with a numeric id of your choosing; `query` returns the ids that fall inside a rectangle.

<!-- BEGIN GENERATED API (scripts/gen_doc.ms; edits between the markers are lost) -->

## quadtree

|Name | Parameters | Purpose |
|-----|------------|---------|
|create |**x**=0, **y**=0, **width**=1, **height**=1 |Create a new quadtree covering the rectangle x, y, width, height; returns the tree |
|insert |**tree**, **id**, **x**, **y** |Add a point with the given id (any number) to the tree; returns 1 if the point was stored, 0 if outside the tree's bounds |
|query |**tree**, **x**, **y**, **width**, **height** |Return a list of the ids of all points inside the rectangle x, y, width, height |
|count |**tree** |Return the number of points stored in the tree |
|clear |**tree** |Remove all points from the tree, keeping its bounds |
<!-- END GENERATED API -->

## Example

```
t = quadtree.create(0, 0, 1000, 1000)
quadtree.insert t, 1, 250, 300
print quadtree.query(t, 200, 250, 100, 100)   // [1]
```
