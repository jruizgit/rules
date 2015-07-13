var r = 2000;

function baseVisual(parent) {
    var that = {};
    
    that.getColor = function (node) {
        if (node.currentState) {
            return '#ff0000';
        }

        return '#fff';
    };

    that.getOpacity = function (node) {
        if (node.currentState) {
            return '.7';
        }

        return '0';
    };

    that.reLayout = function (n, size, yOffset, transpose) {
        var counts = [];
        n.forEach(function (d) {
            if (!counts[d.depth]) {
                counts[d.depth] = 1;
            } else {
                ++counts[d.depth];
            }

            d.order = counts[d.depth] - 1;
        });

        n.forEach(function (d) {
            var xSize = size / counts[d.depth];
            if (transpose) {
                d.x = d.y + yOffset;
                d.y = d.order * xSize + xSize / 2;
            } else {
                d.x = d.order * xSize + xSize / 2;
                d.y = d.y + yOffset;
            }
        });   
    };

    that.getTitle = function (node) {
        var displayEntries = '';
        if (node.currentState) {
            for (var propertyName in node.currentState) {
                if (propertyName !== 'id') {
                    displayEntries = displayEntries + '\t' + propertyName + ':\t' + node.currentState[propertyName] + '\n';
                }
            }

            return displayEntries;
        }

        return '';
    };

    return that;
}

function stateVisual(root, links, x, y, title, parent, state, base) {
    var base = base || baseVisual(parent);
    var that = {};
    var selectedLink = null;
    var zoomedLink = null;
    var pSize = 150;
    var tTime = 0;
    var k = x(r) - x(0);
    k = k / r;
    var n = d3.layout.pack()
        .size([r, r])
        .padding(100)
        .value(function (d) { return d.size; })
        .nodes(root);

    n.forEach(function (d) { d.y = d.y + 25; });

    parent.append('svg:defs').append('svg:marker')
        .attr('id', 'end-arrow')
        .attr('viewBox', '0 -5 10 10')
        .attr('refX', 6)
        .attr('markerWidth', 6)
        .attr('markerHeight', 6)
        .attr('orient', 'auto')
    .append('svg:path')
        .attr('d', 'M0,-5L10,0L0,5')
        .attr('fill', '#ccc');

    var circle = parent.append('svg:g')
        .selectAll('g')
        .data(n);

    var path = parent.append('svg:g')
        .selectAll('path')
        .data(links);

    var pathText = parent.append('svg:g')
        .selectAll('text')
        .data(links);

    if (title) {
        var titleText = parent.append('svg:text')
            .attr('class', 'time label')
            .attr('x', function (d) { return x(30); })
            .attr('y', function (d) { return y(45); })
            .style('font-size', 40 * k + 'px')
            .text(title);
    }

    var popup = parent.append('svg:g')
        .selectAll('g')
        .data(links);

    that.update = function (transitionTime) {
        tTime = transitionTime || 0;

        path.transition()
            .duration(tTime)
            .attr('d', drawLink);

        circle.selectAll('circle').transition()
            .duration(tTime)
            .attr('cx', function (d) { return x(d.x); })
            .attr('cy', function (d) { return y(d.y); })
            .attr('r', function (d) { return k * d.r; })
            .style('fill', base.getColor)
            .style('fill-opacity', base.getOpacity);

        circle.selectAll('title')
            .text(base.getTitle);

        circle.selectAll('text')
            .transition()
            .duration(tTime)
            .attr('x', function (d) { return x(d.x); })
            .attr('y', function (d) { return y(d.y); })
            .style('font-size', function (d) { return 12 * k + 'px'; });

        popup.selectAll('rect[tag=\'smPopupBack\']')
            .transition()
            .duration(tTime)
            .attr('x', function (d) { return x(d.$refX) + 4; })
            .attr('y', function (d) { return y(d.$refY) + 4; })
            .attr('rx', 8 * k)
            .attr('ry', 8 * k)
            .attr('height', function (d) { return ((d === selectedLink) ? pSize * k : 0) })
            .attr('width', function (d) { return ((d === selectedLink) ? pSize * k : 0) });

        popup.selectAll('rect[tag=\'smPopup\']')
            .transition()
            .duration(tTime)
            .attr('x', function (d) { return x(d.$refX); })
            .attr('y', function (d) { return y(d.$refY); })
            .attr('rx', 8 * k)
            .attr('ry', 8 * k)
            .attr('height', function (d) { return ((d === selectedLink) ? pSize * k : 0) })
            .attr('width', function (d) { return ((d === selectedLink) ? pSize * k : 0) });

        popup.each(updateTransition);

    };

    var updateSelection = function (d) {
        popup.style('opacity', function (d1) {
            if (d1 === d) {
                if (selectedLink === d) {
                    selectedLink = null;
                    return '0';
                } else {
                    selectedLink = d;
                    return '1';
                }
            }
            return '0';
        });

        path.classed('selected', function (d) { return d === selectedLink; })

        popup.selectAll('rect[tag=\'smPopupBack\']')
            .attr('x', function (d) { return x(d.$refX) + 4; })
            .attr('y', function (d) { return y(d.$refY) + 4; })
            .attr('rx', 8 * k)
            .attr('ry', 8 * k)
            .attr('height', function (d) { return ((d === selectedLink) ? pSize * k : 0) })
            .attr('width', function (d) { return ((d === selectedLink) ? pSize * k : 0) });

        popup.selectAll('rect[tag=\'smPopup\']')
            .attr('x', function (d) { return x(d.$refX); })
            .attr('y', function (d) { return y(d.$refY); })
            .attr('rx', 8 * k)
            .attr('ry', 8 * k)
            .attr('height', function (d) { return ((d === selectedLink) ? pSize * k : 0) })
            .attr('width', function (d) { return ((d === selectedLink) ? pSize * k : 0) });

        d3.event.stopPropagation();
    };

    var drawLink = function (d) {
        var sourceX, sourceY, targetX, targetY, refX, refY;
        if (d.reflexive) {
            sourceX = d.source.x;
            sourceY = d.source.y - d.source.r;
            targetX = d.source.x + d.source.r;
            targetY = d.source.y;
            refX = d.source.x + d.source.r * (1 + (d.order + 1) * 0.4);
            refY = d.source.y - d.source.r * (1 + (d.order + 1) * 0.4);
            d.$refX = d.source.x + d.source.r * (1 + d.order * 0.2);
            d.$refY = d.source.y - d.source.r * (1 + d.order * 0.2);
        } else if (d.target) {
            var deltaX = Math.abs(d.target.x - d.source.x),
			deltaY = Math.abs(d.target.y - d.source.y),
			dist = Math.sqrt(deltaX * deltaX + deltaY * deltaY),
			orderFactor = (d.order % 2 ? -1 : 1) * (Math.floor(d.order / 2) + 1),
			angle = Math.atan(deltaY / deltaX);
            refX = Math.cos(angle) * dist / 2 * (d.source.x > d.target.x ? -1 : 1) + Math.sin(angle) * dist * orderFactor / 8 * (d.source.y < d.target.y ? -1 : 1) + d.source.x;
            refY = Math.sin(angle) * dist / 2 * (d.source.y > d.target.y ? -1 : 1) + Math.cos(angle) * dist * orderFactor / 8 * (d.source.x > d.target.x ? -1 : 1) + d.source.y;

            var deltaSourceX = Math.abs(refX - d.source.x),
			deltaSourceY = Math.abs(refY - d.source.y),
			sourceAngle = Math.atan(Math.abs(deltaSourceY / deltaSourceX)),
			deltaTargetX = Math.abs(refX - d.target.x),
			deltaTargetY = Math.abs(refY - d.target.y),
			targetAngle = Math.atan(Math.abs(deltaTargetY / deltaTargetX)),
			sourceR = d.left ? d.source.r : d.source.r,
			targetR = d.right ? d.target.r + 5 : d.target.r + 5;
            sourceX = d.source.x + Math.cos(sourceAngle) * sourceR * (d.source.x === d.target.x ? (d.source.y > d.target.y ? -1 : 1) : d.source.x > d.target.x ? -1 : 1);
            sourceY = d.source.y + Math.sin(sourceAngle) * sourceR * (d.source.y === d.target.y ? (d.source.x > d.target.x ? -1 : 1) : d.source.y > d.target.y ? -1 : 1);
            targetX = d.target.x + Math.cos(targetAngle) * targetR * (d.source.x === d.target.x ? (d.source.y > d.target.y ? -1 : 1) : d.source.x > d.target.x ? 1 : -1);
            targetY = d.target.y + Math.sin(targetAngle) * targetR * (d.source.y === d.target.y ? (d.source.x > d.target.x ? -1 : 1) : d.source.y > d.target.y ? 1 : -1);
            d.$refX = refX;
            d.$refY = refY;
        } else {
            return '';
        }

        d.$startX = sourceX;
        d.$startY = sourceY;
        return 'M' + x(sourceX) + ',' + y(sourceY) + 'Q' + x(refX) + ',' + y(refY) + ',' + x(targetX) + ',' + y(targetY);
    };

    var zoom = function (d) {
        if (zoomedLink !== d) {
            x = d3.scale.linear().range([25 * k, (r - 25) * k]);
            y = d3.scale.linear().range([25 * k, (r - 25) * k]);
            x.domain([d.$refX, d.$refX + pSize]);
            y.domain([d.$refY, d.$refY + pSize]);
            k = k * (r - 50) / pSize;
            zoomedLink = d;
            that.update(1000);
        } else {
            k = (k * pSize) / (r - 50);
            x = d3.scale.linear().range([0, r * k]);
            y = d3.scale.linear().range([0, r * k]);
            x.domain([0, r]);
            y.domain([0, r]);
            zoomedLink = null;
            that.update(1000);
        }
        d3.event.stopPropagation();
    };

    var drawTransition = function (d) {
        if (d.nodes) {
            var newX = d3.scale.linear().range([x(d.$refX), x(d.$refX + pSize)]);
            var newY = d3.scale.linear().range([y(d.$refY), y(d.$refY + pSize)]);
            newX.domain([0, r]);
            newY.domain([0, r + 50]);
            d.update = ruleVisual(d.nodes[0], d.links, newX, newY, d.id, d3.select(this), false, base).update;
        }
    };

    var updateTransition = function (d) {
        if (d.nodes) {
            var newX = d3.scale.linear().range([x(d.$refX), x(d.$refX + pSize)]);
            var newY = d3.scale.linear().range([y(d.$refY), y(d.$refY + pSize)]);
            newX.domain([0, r]);
            newY.domain([0, r + 50]);
            d.update(tTime, newX, newY);
        }
    };

    path.enter()
        .append('svg:path')
        .attr('class', 'link')
        .attr('id', function (d) { return d.source.id + d.id; })
        .style('marker-end', function (d) { return 'url(#end-arrow)'; })
        .on('click', updateSelection)
        .attr('d', drawLink);

    pathText.enter()
        .append('svg:text')
        .attr('class', 'linkDisplay')
        .attr('dy', -5)
        .on('click', updateSelection)
        .append('svg:textPath')
        .attr('xlink:href', function (d) { return '#' + d.source.id + d.id })
        .attr('startOffset', '5%')
        .text(function (d) { return d.id })
        .style('font-size', function (d) { return 12 * k + 'px'; });

    var g = circle.enter().append('svg:g');

    g.append('svg:circle')
        .attr('class', 'node')
		.attr('cx', function (d) { return x(d.x); })
        .attr('cy', function (d) { return y(d.y); })
        .attr('r', function (d) { return k * d.r })
        .style('opacity', function (d) { return (d === root) ? '0' : '1'; });

    g.append('title');

    g.append('svg:text')
        .attr('class', 'id')
        .attr('text-anchor', 'middle')
        .attr('x', function (d) { return x(d.x); })
        .attr('y', function (d) { return y(d.y); })
        .text(function (d) { return d.id; })
        .style('font-size', function (d) { return 12 * k + 'px'; });

    g = popup.enter()
        .append('svg:g')
        .style('opacity', '0');

    g.append('svg:rect')
        .attr('class', 'popupBack')
        .attr('tag', 'smPopupBack');

    g.append('svg:rect')
        .attr('class', 'popup')
        .attr('tag', 'smPopup')
        .on('click', zoom);

    popup.each(drawTransition);

    state.onStateChanged(function () {
        that.update(500)
    });

    d3.select(window).on('click', function () {
        if (zoomedLink !== null) {
            zoom(zoomedLink);
        } else if (selectedLink !== null) {
            updateSelection(selectedLink);
        }
    });

    return that;
}


function flowVisual(root, links, x, y, title, parent, state, base) {
    var base = base || baseVisual(parent);
    var that = {};
    var selectedLink = null;
    var zoomedLink = null;
    var pSize = 150;
    var tTime = 0;
    var k = x(r) - x(0);
    k = k / r;

    var n = d3.layout.tree()
        .size([r, r])
        .nodes(root);

    base.reLayout(n, r, 45);

    parent.append('svg:defs').append('svg:marker')
        .attr('id', 'end-arrow')
        .attr('viewBox', '0 -5 10 10')
        .attr('refX', 6)
        .attr('markerWidth', 6)
        .attr('markerHeight', 6)
        .attr('orient', 'auto')
    .append('svg:path')
        .attr('tag', 'arrow')
        .attr('d', 'M0,-5L10,0L0,5')
        .attr('fill', '#ccc');

    var path = parent.append('svg:g')
        .selectAll('path')
        .data(links);

    var pathText = parent.append('svg:g')
        .selectAll('text')
        .data(links);

    var step = parent.append('svg:g')
        .selectAll('g')
        .data(n);

    if (title) {
        var titleText = parent.append('svg:text')
            .attr('class', 'time label')
            .attr('x', function (d) { return x(30); })
            .attr('y', function (d) { return y(45); })
            .style('font-size', 40 * k + 'px')
            .text(title);
    }

    var popup = parent.append('svg:g')
        .selectAll('g')
        .data(links);

    var getSource = function (d) {
        var deltaX = Math.abs(d.target.x - d.source.x),
        deltaY = Math.abs(d.target.y - d.source.y),
        sourceR = d.source.r / 2 + 2,
        sourceX = d.source.x,
        sourceY = d.source.y;
        // aligned vertically, 
        if (!deltaX) {
            // only if pointing upwards
            if (d.source.y > d.target.y) {
                // offset the origin
                sourceX += sourceR;
            }
        } else {
            //if source on the right
            if (d.source.x > d.target.x) {
                sourceX -= sourceR;
            } else {
                sourceX += sourceR;
            }
        }

        // not a switch
        if (!d.source.condition) {
            // not aligned vertically or pointing downwards
            if (deltaX || (d.source.y < d.target.y)) {
                //if source below
                if (d.source.y > d.target.y) {
                    sourceY -= sourceR;
                } else {
                    sourceY += sourceR;
                }
            }
        } else {
            // aligned vertically and pointing downwards
            if (!deltaX && (d.source.y < d.target.y)) {
                //if source below
                if (d.source.y > d.target.y) {
                    sourceY -= sourceR;
                } else {
                    sourceY += sourceR;
                }
            }
        }

        d.$refX = d.source.x + (d.target.x - d.source.x)/2;
        d.$refY = d.source.y + (d.target.y - d.source.y)/2;
        return { x: x(sourceX), y: y(sourceY) };
    }

    var getTarget = function (d) {
        var deltaX = Math.abs(d.target.x - d.source.x),
        deltaY = Math.abs(d.target.y - d.source.y),
        targetR = d.target.r / 2 + 2,
        targetX = d.target.x + targetR * ((!deltaX && d.source.y > d.target.y) ? 1 : 0),
        targetY = d.target.y + targetR * (d.source.y > d.target.y ? 1 : -1);
        return { x: x(targetX), y: y(targetY) };
    }

    var diagonal = d3.svg.diagonal()
        .source(getSource)
        .target(getTarget);

    that.update = function (transitionTime) {
        tTime = transitionTime || 0;

        path.transition()
            .duration(tTime)
            .attr('d', diagonal);

        step.selectAll('rect').transition()
            .duration(tTime)
            .attr('x', function (d) { return x(d.x - d.r / 2 / ((d.condition) ? Math.sqrt(2) : 1)); })
            .attr('y', function (d) { return y(d.y - d.r / 2 / ((d.condition) ? Math.sqrt(2) : 1)); })
            .attr('height', function (d) { return (d.r / ((d.condition) ? Math.sqrt(2) : 1)) * k; })
            .attr('width', function (d) { return (d.r / ((d.condition) ? Math.sqrt(2) : 1)) * k; })
            .style('fill', base.getColor)
            .style('fill-opacity', base.getOpacity)
            .attr('transform', function (d) { return (d.condition) ? 'rotate(45,' + x(d.x) + ',' + y(d.y) + ')' : ''; });

        step.selectAll('title')
            .text(base.getTitle);

        step.selectAll('text')
            .transition()
            .duration(tTime)
            .attr('x', function (d) { return x(d.x); })
            .attr('y', function (d) { return y(d.y + 3); })
            .style('font-size', function (d) { return 12 * k + 'px'; });

        popup.selectAll('rect[tag=\'fcPopupBack\']')
            .transition()
            .duration(tTime)
            .attr('x', function (d) { return x(d.$refX) + 4; })
            .attr('y', function (d) { return y(d.$refY) + 4; })
            .attr('rx', 8 * k)
            .attr('ry', 8 * k)
            .attr('height', function (d) { return ((d === selectedLink) ? pSize * k : 0) })
            .attr('width', function (d) { return ((d === selectedLink) ? pSize * k : 0) });

        popup.selectAll('rect[tag=\'fcPopup\']')
            .transition()
            .duration(tTime)
            .attr('x', function (d) { return x(d.$refX); })
            .attr('y', function (d) { return y(d.$refY); })
            .attr('rx', 8 * k)
            .attr('ry', 8 * k)
            .attr('height', function (d) { return ((d === selectedLink) ? pSize * k : 0) })
            .attr('width', function (d) { return ((d === selectedLink) ? pSize * k : 0) });

        popup.each(updateTransition);
    }

    var updateSelection = function (d) {
        popup.style('opacity', function (d1) {
            if (d1 === d) {
                if (selectedLink === d) {
                    selectedLink = null;
                    return '0';
                } else {
                    selectedLink = d;
                    return '1';
                }
            }
            return '0';
        });

        path.classed('selected', function (d) { return d === selectedLink; })

        popup.selectAll('rect[tag=\'fcPopupBack\']')
            .attr('x', function (d) { return x(d.$refX) + 4; })
            .attr('y', function (d) { return y(d.$refY) + 4; })
            .attr('rx', 8 * k)
            .attr('ry', 8 * k)
            .attr('height', function (d) { return ((d === selectedLink) ? pSize * k : 0) })
            .attr('width', function (d) { return ((d === selectedLink) ? pSize * k : 0) });

        popup.selectAll('rect[tag=\'fcPopup\']')
            .attr('x', function (d) { return x(d.$refX); })
            .attr('y', function (d) { return y(d.$refY); })
            .attr('rx', 8 * k)
            .attr('ry', 8 * k)
            .attr('height', function (d) { return ((d === selectedLink) ? pSize * k : 0) })
            .attr('width', function (d) { return ((d === selectedLink) ? pSize * k : 0) });

        d3.event.stopPropagation();
    };

    var drawTransition = function (d) {
        if (d.nodes) {
            var newX = d3.scale.linear().range([x(d.$refX), x(d.$refX + pSize)]);
            var newY = d3.scale.linear().range([y(d.$refY), y(d.$refY + pSize)]);
            newX.domain([0, r]);
            newY.domain([0, r + 50]);
            d.update = ruleVisual(d.nodes[0], d.links, newX, newY, d.id, d3.select(this), false, base).update;
        }
    };

    var updateTransition = function (d) {
        if (d.nodes) {
            var newX = d3.scale.linear().range([x(d.$refX), x(d.$refX + pSize)]);
            var newY = d3.scale.linear().range([y(d.$refY), y(d.$refY + pSize)]);
            newX.domain([0, r]);
            newY.domain([0, r + 50]);
            d.update(tTime, newX, newY);
        }
    };

    var zoom = function (d) {
        if (zoomedLink !== d) {
            x = d3.scale.linear().range([25 * k, (r - 25) * k]);
            y = d3.scale.linear().range([25 * k, (r - 25) * k]);
            x.domain([d.$refX, d.$refX + pSize]);
            y.domain([d.$refY, d.$refY + pSize]);
            k = k * (r - 50) / pSize;
            zoomedLink = d;
            that.update(1000);
        }
        else {
            k = (k * pSize) / (r - 50);
            x = d3.scale.linear().range([0, r * k]);
            y = d3.scale.linear().range([0, r * k]);
            x.domain([0, r]);
            y.domain([0, r]);
            zoomedLink = null;
            that.update(1000);
        }
        d3.event.stopPropagation();
    };


    path.enter()
        .append('svg:path')
        .attr('class', 'link')
        .attr('id', function (d) { return d.source.x + '-' + d.source.y + '-' + d.target.x + '-' + d.target.y;})
        .style('marker-end', function (d) { return 'url(#end-arrow)'; })
        .on('click', updateSelection)
        .attr('d', diagonal);

    pathText.enter()
        .append('svg:text')
        .attr('class', 'linkDisplay')
        .attr('dy', -5)
        .on('click', updateSelection)
        .append('svg:textPath')
        .attr('xlink:href', function (d) { return '#' + d.source.x + '-' + d.source.y + '-' + d.target.x + '-' + d.target.y;})
        .attr('startOffset', '5%')
        .text(function (d) { return d.id })
        .style('font-size', function (d) { return 12 * k + 'px'; });

    var g = step.enter().append('svg:g');

    g.append('svg:rect')
        .attr('class', 'node')
        .attr('x', function (d) { return x(d.x - d.r / 2 / ((d.condition) ? Math.sqrt(2) : 1)); })
        .attr('y', function (d) { return y(d.y - d.r / 2 / ((d.condition) ? Math.sqrt(2) : 1)); })
        .attr('height', function (d) { return (d.r / ((d.condition) ? Math.sqrt(2) : 1)) * k; })
        .attr('width', function (d) { return (d.r / ((d.condition) ? Math.sqrt(2) : 1)) * k; })
        .attr('transform', function (d) { return (d.condition) ? 'rotate(45,' + x(d.x) + ',' + x(d.y) + ')' : ''; });

    g.append('title');

    g.append('svg:text')
        .attr('class', 'id')
        .attr('text-anchor', 'middle')
        .attr('x', function (d) { return x(d.x); })
        .attr('y', function (d) { return y(d.y + 3); })
        .style('font-size', function (d) { return 12 * k + 'px'; })
        .text(function (d) { return d.id; });

    g = popup.enter()
        .append('svg:g')
        .style('opacity', '0');

    g.append('svg:rect')
        .attr('class', 'popupBack')
        .attr('tag', 'fcPopupBack');

    g.append('svg:rect')
        .attr('class', 'popup')
        .attr('tag', 'fcPopup')
        .on('click', zoom);

    popup.each(drawTransition);

    state.onStateChanged(function () {
        that.update(500)
    });

    d3.select(window).on('click', function () {
        if (zoomedLink !== null) {
            zoom(zoomedLink);
        } else if (selectedLink !== null) {
            updateSelection(selectedLink);
        }
    });

    return that;
}

function rulesetVisual(roots, links, x, y, title, parent, state, base) {
    var base = base || baseVisual(parent);
    var that = {};
    var visuals = [];
    var zoomedSelection;
    var selection;
    var rulesLength = Object.keys(roots).length;
    
    var currentY = 0;
    var linkHash = links[0];
    for (var ruleName in roots) {
        var currentRoot = roots[ruleName];
        var currentLinkArray = linkHash[ruleName];
        var newX = d3.scale.linear().range([0, x(r / rulesLength)]);
        var newY = d3.scale.linear().range([y(currentY), y(currentY + r / rulesLength)]);
        newX.domain([0, r]);
        newY.domain([0, r]);
        visuals.push(ruleVisual(currentRoot, currentLinkArray, newX, newY, ruleName, parent, true, base).update);   
        currentY += r / rulesLength;
    }
    
    var zoom = function () {
        var ruleSize = r / rulesLength;
        var selection = Math.floor(d3.mouse(this)[1] / ruleSize);
        var currentVisual = visuals[selection];
        var newX;
        var newY;
        if (zoomedSelection !== selection) {
            for (var i = 0; i < visuals.length; ++i) {
                newX = d3.scale.linear().range([0, ruleSize]);
                newY = d3.scale.linear().range([r * (i - selection), r * (i - selection) + ruleSize]);
                newX.domain([0, ruleSize]);
                newY.domain([0, ruleSize]);
                visuals[i](1000, newX, newY);
            }

            zoomedSelection = selection;
        } else {
            for (var i = 0; i < visuals.length; ++i) {
                newX = d3.scale.linear().range([0, ruleSize]);
                newY = d3.scale.linear().range([ruleSize * i, ruleSize * (i + 1)]);
                newX.domain([0, r]);
                newY.domain([0, r]);
                visuals[i](1000, newX, newY);
            }

            zoomedSelection = null;
        }
        d3.event.stopPropagation();
    }

    parent.on('click', zoom);
    return that;
}

function ruleVisual(root, links, x, y, title, parent, transpose, base) {
    var base = base || baseVisual(parent);
    var that = {};
    var pSize = 150;
    var k = x(r) - x(0);
    k = k / r;
    var n = d3.layout.tree()
        .size([r, r])
        .nodes(root);

    var offset = -400 + n.length * 80;
    if (offset > 0) {
        offset = 0;
    }
    base.reLayout(n, r, offset, transpose);

    parent.append('svg:defs').append('svg:marker')
        .attr('id', 'end-arrow')
        .attr('viewBox', '0 -5 10 10')
        .attr('refX', 6)
        .attr('markerWidth', 6)
        .attr('markerHeight', 6)
        .attr('orient', 'auto')
    .append('svg:path')
        .attr('d', 'M0,-4L8,0L0,4')
        .attr('fill', '#ccc');

    var path = parent.append('svg:g')
        .selectAll('path')
        .data(links);

    var step = parent.append('svg:g')
        .selectAll('g')
        .data(n);

    if (title) {
        var titleText = parent.append('svg:text')
            .attr('class', 'time label')
            .attr('x', function (d) { return x(30); })
            .attr('y', function (d) { return y(45); })
            .style('font-size', 40 * k + 'px')
            .text(title);
    }

    var getSource = function (d) {
        if (transpose) {
            sourceX = (d.source.x + d.source.r);
            sourceY = d.source.y;
        } else {
            sourceX = d.source.x;
            sourceY = (d.source.y + d.source.r);
        } 
        return { x: x(sourceX), y: y(sourceY) };
    }

    var getTarget = function (d) {
        if (transpose) {
            if (d.source.y < d.target.y) {
                // pointing down
                targetY = d.target.y - d.target.r;  
                targetX = d.target.x; 
            } else if (d.source.y > d.target.y) {
                // pointing up
                targetY = d.target.y + d.target.r; 
                targetX = d.target.x;
            } else {
                // aligned
                targetY = d.target.y;
                targetX = d.target.x - d.target.r - 5;
            }            
        } else {
            targetX = d.target.x;
            targetY = d.target.y - d.target.r;
        }
        return { x: x(targetX), y: y(targetY) };
    }

    var diagonal = d3.svg.diagonal()
        .source(getSource)
        .target(getTarget);

    var drawSymbol = function (d) {
        if (d.type === 'op') {
            return d3.svg.symbol()
                     .type('circle')
                     .size(d.r * d.r * 3.14 / 2)(d);
        }

        if (d.type === 'input') {
            var h = d.r * 2 / 3;
            return 'M ' + (d.r * -1) + ' 0 L ' + (h * -1) + ' ' + (h * -1) + ' L ' + h + ' ' + (h * -1) + ' L ' + d.r 
                        + ' 0 L ' + h + ' ' + h + ' L ' + (h * -1) + ' ' + h + ' z';
        }

        if (d.type === 'function') {
            var h = d.r * 2 / 3;
            return 'M ' + (d.r * -1) + ' ' + (h * -1) + ' L ' + d.r + ' ' + (h * -1) +  ' L ' + d.r + ' ' + h + ' L ' 
                        + (d.r * -1) + ' ' + h + ' z';
        }

        return;
    };

    var getColor = function(d) {
        if (d.type === 'op') {
            return d3.rgb(236,236,236); 
        }

        if (d.type === 'input') {
            return d3.rgb(252,99,93);  
        }

        if (d.type === 'function') {
            return d3.rgb(95,191,96);  
        }
    };

    var getTextX = function(d) {
        if (d.type === 'op') {
            return x(d.x - d.r - d.exp.length * 6);
        }

        if (d.exp) {
           return x(d.x - d.exp.length / 2 * 6);
        }
        return x(d.x - d.r);
    };

    var getTextY = function(d) {
        if (d.type === 'op') {
            return y(d.y + 12 * k / 2);
        }

        return y(d.y - d.r);
    };

    that.update = function (transitionTime, newX, newY) {
        x = newX || x;
        y = newY || y;
        k = x(r) - x(0);
        k = k / r;
        transitionTime = transitionTime || 0;

        path.transition()
            .duration(transitionTime)
            .attr('d', diagonal);

        step.selectAll('path')
            .transition()
            .duration(transitionTime)
            .attr('transform', function (d) { return 'translate(' + x(d.x) + ',' + y(d.y) + '),scale(' + k + ')'; });

        step.selectAll('title')
            .text(base.getTitle);

        step.selectAll('text')
            .transition()
            .duration(transitionTime)
            .attr('x', getTextX)
            .attr('y', getTextY)
            .style('font-size', function (d) { return 12 * k + 'px'; });

        if (titleText) {
            titleText.transition()
                .duration(transitionTime)
                .attr('x', function (d) { return x(30); })
                .attr('y', function (d) { return y(45); })
                .style('font-size', 40 * k + 'px');
        }
    };

    path.enter()
        .append('svg:path')
        .attr('class', 'link')
        .style('marker-end', function (d) { return 'url(#end-arrow)'; })
        .attr('d', diagonal);

    var g = step.enter().append('svg:g');

    g.append('svg:path')
        .attr('class', 'sequenceNode')
        .attr('transform', function (d) { return 'translate(' + x(d.x) + ',' + y(d.y) + '),scale(' + k + ')'; })
        .attr('d', drawSymbol)
        .style('fill', getColor)
        .style('fill-opacity', '.7');

    g.append('title');

    g.append('svg:text')
        .attr('class', 'id')
        .attr('text-anchor', 'right')
        .attr('x', getTextX)
        .attr('y', getTextY)
        .style('font-size', function (d) { return 12 * k + 'px'; })
        .text(function (d) { return d.exp; });

    return that;
}

function rulesetState(url) {
    var that = {};
    var stateChangedEvents = [];
    var errorEvents = [];
    var currentState;
    var sid = url.substring(0, url.lastIndexOf('/'));
    var rulesetUrl = sid.substring(0, sid.lastIndexOf('/'));
    sid = sid.substring(sid.lastIndexOf('/') + 1);

    that.onStateChanged = function (stateChangedFunc) {
        stateChangedEvents.push(stateChangedFunc);
        return that;
    }

    that.onError = function (errorFunc) {
        errorEvents.push(errorFunc);
        return that;
    }

    that.getCurrentState = function () {
        return currentState;
    }

    var update = function () {
        var i;
        var statusUrl = rulesetUrl + '/' + sid;
        d3.json(statusUrl, function (err, status) {
            if (err) {
                for (i = 0; i < errorEvents.length; ++i) {
                    errorEvents[i](err.responseText);
                }
            } else {
                if (JSON.stringify(currentState) !== JSON.stringify(status)) {
                    currentState = status;

                    for (i = 0; i < stateChangedEvents.length; ++i) {
                        stateChangedEvents[i](currentState, sid);
                    }
                }
            }
        });
    }

    var timeout = function () {
        update();
        setTimeout(timeout, 5000);
    }

    that.start = function () {
        timeout();
    }

    return that;
}

function rulesetGraph(url, state) {
    var that = {};
    var links = [];
    var nodes;
    var nodeDictionary = {};
    var sid = url.substring(0, url.lastIndexOf('/'));
    var ruleSetUrl = sid.substring(0, sid.lastIndexOf('/'));
    sid= sid.substring(sid.lastIndexOf('/') + 1);

    if (sid.indexOf('.') !== -1) {
        sid = sid.substring(0, sid.indexOf('.'));
    }

    var getExpression = function(expression, typeName) {
        var operator;
        var lop;
        var rop;
        var result = '';
        var expressions;
        var i;
        var atLeast;
        var atMost;
        var skipParens = false;

        if (!typeName.indexOf('$')) {
            typeName = typeName.substring(1, typeName.length);
        }

        for (var opName in expression) {
            if (opName === '$atLeast') {
                atLeast = expression[opName];
            } else if (opName === '$atMost') {
                atMost = expression[opName];
            } else {
                operator = opName;
            }
        }

        if (operator === '$ne' || operator === '$lte' || operator === '$lt' ||
            operator === '$gte' || operator === '$gt' || operator === '$nex' || operator === '$ex') {
            var operands = expression[operator];
            for (lop in operands) {
                break;
            }

            rop = operands[lop];
            if (typeof (rop) === 'object') {
                ropo = rop['$s'];
                if (typeof (ropo) === 'object') {
                    rop = 's';
                    if (ropo['id']) {
                        rop = rop + '.id(\'' + ropo['id'] + '\')';
                    }
                    if (ropo['time']) {
                        rop = rop + '.time(' + ropo['time'] + ')';
                    }

                    rop = rop + '.'  + ropo['name'];
                } else {
                    rop = 's.' + ropo;
                }
            }
        }

        switch (operator) {
            case '$ne':
                result = typeName + '.' + lop + ' <> ' + rop;
                break;
            case '$lte':
                result = typeName + '.' + lop + ' <= ' + rop;
                break;
            case '$lt':
                result = typeName + '.' + lop + ' < ' + rop;
                break;
            case '$gte':
                result = typeName + '.' + lop + ' >= ' + rop;
                break;
            case '$gt':
                result = typeName + '.' + lop + ' > ' + rop;
                break;
            case '$ex':
                result = 'ex(' + typeName + '.' + lop + ')';
                skipParens = true;
                break;
            case '$nex':
                result = 'nex(' + typeName + '.' + lop + ')';
                skipParens = true;
                break;
            case '$t':
                result = 'timeout(\'' + expression[operator] + '\')';
                skipParens = true;
                break;
            case '$and':
                expressions = expression[operator];
                for (i = 0; i < expressions.length; ++i) {
                    if (i !== 0) {
                        result = result + ' & ';
                    }
                    result = result + getExpression(expressions[i], typeName);
                }
                break;
            case '$or':
                expressions = expression[operator];
                for (i = 0; i < expressions.length; ++i) {
                    if (i !== 0) {
                        result = result + ' | ';
                    }
                    result = result + getExpression(expressions[i], typeName);
                }
                break;
            default:
                rop = expression[operator];
                result = typeName + '.' + operator + ' == ' + rop;
        }

        if (!skipParens) {
            result = '(' + result + ')';
        }

        if (atLeast) {
            result = result + '.at_least(' + atLeast + ')'
        }

        if (atMost) {
            result = result + '.at_most(' + atMost + ')'
        }        

        return result;
    };

    var getStateNodes = function (chart, links, parentId) {
        var currentState;
        var resultNode = { size: 20, id: '', currentState: null, children: [] };
        var stateNode;
        var stateId;
        var stateName;
        var linkCounter = {};

        for (stateName in chart) {
            currentState = chart[stateName];
            if (parentId) {
                stateId = parentId + '.' + stateName;
            } else {
                stateId = stateName;
            }

            if (currentState.$chart) {
                stateNode = getStateNodes(currentState.$chart, links, stateId);
                stateNode.id = stateId;
            } else {
                stateNode = { id: stateId, currentState: null, size: 20 };
            }

            nodeDictionary[stateId] = stateNode;
            resultNode.children.push(stateNode);
        }

        for (stateName in chart) {
            currentState = chart[stateName];
            if (parentId) {
                stateId = parentId + '.' + stateName;
            } else {
                stateId = stateName;
            }

            for (var transitionName in currentState) {
                var transition = currentState[transitionName];
                if (transition.to) {
                    var transitionNode = { r: 25, type: 'start', top: true, children: [] };
                    var transitionLinks = [];
                    var outputNode;
                    var transitionId;
                    
                    if (parentId) {
                        transitionId = parentId + '.' + transitionName;
                    } else {
                        transitionId = transitionName;
                    }

                    outputNode = getRuleNodes(transitionId, transition, transitionNode, transitionLinks);
                    var link = {
                        source: nodeDictionary[stateId],
                        target: nodeDictionary[transition.to],
                        nodes: [transitionNode, outputNode],
                        links: transitionLinks,
                        id: transitionId,
                        left: false,
                        right: true
                    };

                    if (transition.to === stateId) {
                        link.reflexive = true;
                    }

                    var counter = linkCounter[stateId + transition.to];
                    if (counter) {
                        counter.count = counter.count + 1;
                        link.order = counter.count;
                    }
                    else {
                        linkCounter[stateId + transition.to] = { count: 0 };
                        link.order = 0;
                    }

                    links.push(link);
                }
            }
        }

        resultNode.type = 'stateChart';
        return resultNode;
    };


    var getFlowNodes = function (chart, links) {
        var resultNode;
        var visitedNodes = {};
        for (var stageName in chart) {
            var currentStage = chart[stageName];
            var stageNode = { id: stageName, r: 50, currentStage: null, children: [] };
            nodeDictionary[stageName] = stageNode;
            if (!resultNode) {
                resultNode = stageNode;
                visitedNodes[stageName] = stageNode;
            }
        }

        var targetNode;
        var sourceNode;
        var currentStage;
        for (var stageName in chart) {
            currentStage = chart[stageName];
            sourceNode = nodeDictionary[stageName];
            if (currentStage.to) {
                if (typeof (currentStage.to) === 'string') {
                    targetNode = nodeDictionary[currentStage.to];
                    if (targetNode) {
                        if (!visitedNodes[currentStage.to]) {
                            sourceNode.children.push(targetNode);
                            visitedNodes[currentStage.to] = targetNode;
                        }

                        links.push({ source: sourceNode, target: targetNode, left: false, right: true });
                    }
                }
                else if (Object.keys(currentStage.to).length) {
                    var conditionNode = { id: 'switch', r: 50, children: [], condition: true };
                    sourceNode.children.push(conditionNode);
                    links.push({ source: sourceNode, target: conditionNode, left: false, right: true });
                    for (var targetName in currentStage.to) {
                        targetNode = nodeDictionary[targetName];
                        if (targetNode) {
                            var transition = currentStage.to[targetName];
                            var transitionNode = { r: 25, type: 'start', top: true, children: [] };
                            var transitionLinks = [];
                            var outputNode;
                            outputNode = getRuleNodes(targetName, {when: transition}, transitionNode, transitionLinks);
                            
                            var link = {
                                source: conditionNode,
                                target: targetNode,
                                nodes: [transitionNode, outputNode],
                                links: transitionLinks,
                                id: targetName,
                                left: false,
                                right: true
                            };

                            if (!visitedNodes[targetName]) {
                                conditionNode.children.push(targetNode);
                                visitedNodes[targetName] = targetNode;
                            }

                            links.push(link);
                        }
                    }
                }
            }
        }

        resultNode.type = 'flowChart';
        return resultNode;
    };

    var getRulesetNodes = function (ruleset, links) {
        var topNode;
        var rule;
        var roots = {};
        var linkHash = {};
        var linkArray;
        for (var ruleName in ruleset) {
            linkArray = [];
            rule = ruleset[ruleName];
            rule.run = '#';
            topNode = { r: 25, type: 'start', top: true, children: [] };
            getRuleNodes(ruleName, rule, topNode, linkArray);
            roots[ruleName] = topNode;
            linkHash[ruleName] = linkArray;
        }

        links.push(linkHash);
        return roots;
    }

    var getRuleNodes = function (ruleName, rule, parentNode, links) {
        var outputNode;
        if (rule.whenAny) {
            outputNode = getAlgebraNodes(parentNode, rule.whenAny, 'any', links);
        } else if (rule.whenAll) {
            outputNode = getAlgebraNodes(parentNode, rule.whenAll, 'all', links);
        } else if (rule.when) {
            var typeName;
            for (typeName in rule.when) {
                break;
            }

            if (typeName.indexOf('$all') !== -1) {
                outputNode = getAlgebraNodes(parentNode, rule.when[typeName], 'all', links);
            } else if (typeName.indexOf('$any') !== -1) {
                outputNode = getAlgebraNodes(parentNode, rule.when[typeName], 'any', links);
            } else {
                outputNode = { r: 25, top: true, type: 'input' ,children: [] };
                
                if (typeName === '$m' || typeName === '$s') {
                    outputNode.exp = getExpression(rule.when[typeName], typeName);
                } else {
                    outputNode.exp = getExpression(rule.when, '$m');
                }

                parentNode.children.push(outputNode);
            }
        } else {
            outputNode = parentNode;
        }

        if (rule.run) {
            var runNode = { r: 25, exp: ruleName, type: 'function', top: true, children: [] };
            outputNode.children.push(runNode);
            links.push({ source: outputNode, target: runNode, left: false, right: true });
        }

        return outputNode;
    }

    var getAlgebraNodes = function (rootNode, events, op, links) {
        var endNode = { r: 25, type: 'op', children: [] };
        var endNodes = [];
        var currentNode;
        var atLeast;
        var atMost;
        for (var eventName in events) {
            var currentExpression = events[eventName];

            if (eventName == '$atLeast') {
                atLeast = currentExpression;
                break;
            } else if (eventName == '$atMost') {
                atMost = currentExpression;
                break;
            } else if (eventName.indexOf('$all') !== -1) {
                currentNode = getAlgebraNodes(rootNode, currentExpression, 'all', links);
            } else if (eventName.indexOf('$any') !== -1) {
                currentNode = getAlgebraNodes(rootNode, currentExpression, 'any', links);
            } else {
                currentNode = { r: 25, type: 'input', children: [] };
                if (eventName !== '$s') {
                    currentNode.exp = getExpression(currentExpression, 'm');
                } else {
                    currentNode.exp = getExpression(currentExpression, 's');
                }
                currentNode.depth = 0;
                rootNode.children.push(currentNode);
            }

            endNodes.push(currentNode);
            links.push({ source: currentNode, target: endNode, left: false, right: true });
        }

        if (atLeast) {
            op = op + '.at_least(' + atLeast + ')'; 
        }

        if (atMost) {
            op = op + '.at_most(' + atMost + ')'; 
        }

        endNode['exp'] = op;

        var maxDepthNode;
        for (var i = 0; i < endNodes.length; ++i) {
            currentNode = endNodes[i];
            if (!maxDepthNode) {
                maxDepthNode = currentNode;
            }
            else {
                if (currentNode.depth > maxDepthNode.depth) {
                    maxDepthNode = currentNode;
                }
            }
        }

        maxDepthNode.children.push(endNode);
        endNode.depth = maxDepthNode.depth + 1;
        return endNode;
    };

    var getNodes = function (ruleset, links) {
        if (ruleset.$type === 'stateChart') {
            delete(ruleset.$type);
            return getStateNodes(ruleset, links);
        }
        else if (ruleset.$type === 'flowChart') {
            delete(ruleset.$type);
            return getFlowNodes(ruleset, links);
        } else {
            return getRulesetNodes(ruleset, links);
        }
    };

    var setCurrentState = function (entry) {
        if (nodeDictionary[entry.label]) {
            nodeDictionary[entry.label].currentState = entry;
        }
    };

    that.createVisual = function (frameSize, callback) {
        d3.json(ruleSetUrl, function (err, ruleSet) {
            if (err) {
                var error;
                try {
                    error = JSON.parse(err.responseText);
                } catch (ex) {
                    error = { error: ex };
                }

                callback(error);
            }
            else {
                var svg = d3.select('body')
                            .append('svg')
                            .attr('width', r + 200)
                            .attr('height', r + 75);

                var x;
                var y;

                //nodes = getNodes(ruleSet, links);
                if (frameSize) {
                    frameSize = parseInt(frameSize);
                    x = d3.scale.linear().range([0, frameSize]);
                    y = d3.scale.linear().range([0, frameSize]);
                    x.domain([0, r + 50]);
                    y.domain([0, r + 50]);
                } else {
                    x = d3.scale.linear().range([0, r]);
                    y = d3.scale.linear().range([0, r]);
                    x.domain([0, r]);
                    y.domain([0, r]);
                }

                var lines = svg.append('svg:g')
                                .selectAll('g')
                                .data(that.lines);

                lines.enter()
                     .append('svg:line')
                     .attr('x1', function(d) { return Math.floor(d.p1 / 100) / 10; })
                     .attr('y1', function(d) { return 10 * (d.p1 % 100); })
                     .attr('x2', function(d) { return Math.floor(d.p2 / 100) / 10; })
                     .attr('y2', function(d) { return 10 * (d.p2 % 100); })
                     .style('stroke', 'rgb(6,120,155)');


                // var title = ruleSetUrl.substring(1) + '/' + sid;
                // if (nodes.type === 'stateChart') {
                //     stateVisual(nodes, links, x, y, title, svg, state);
                // } else if (nodes.type === 'flowChart') {
                //     flowVisual(nodes, links, x, y, title, svg, state);
                // } else {
                //     rulesetVisual(nodes, links, x, y, title, svg, state);
                // }

                // state.start();
                // if (callback) {
                //     callback();
                // }
            }
        });
    };

    that.lines = [];
    var appendLine = function (line) {
        that.lines.push(line);
    }
    appendLine({'t':'line' ,'p1':50003 ,'p2':60003})
    appendLine({'t':'line' ,'p1':30005 ,'p2':30006})
    appendLine({'t':'line' ,'p1':80005 ,'p2':80006})
    appendLine({'t':'line' ,'p1':50008 ,'p2':60008})
    appendLine({'t':'line' ,'p1':0 ,'p2':20000})
    appendLine({'t':'line' ,'p1':20000 ,'p2':30000})
    appendLine({'t':'line' ,'p1':30000 ,'p2':40000})
    appendLine({'t':'line' ,'p1':0 ,'p2':2})
    appendLine({'t':'line' ,'p1':2 ,'p2':3})
    appendLine({'t':'line' ,'p1':3 ,'p2':4})
    appendLine({'t':'line' ,'p1':4 ,'p2':40004})
    appendLine({'t':'line' ,'p1':40004 ,'p2':40000})
    appendLine({'t':'line' ,'p1':40000 ,'p2':50001})
    appendLine({'t':'line' ,'p1':50001 ,'p2':50002})
    appendLine({'t':'line' ,'p1':50002 ,'p2':50003})
    appendLine({'t':'line' ,'p1':50003 ,'p2':50005})
    appendLine({'t':'line' ,'p1':50005 ,'p2':40004})
    appendLine({'t':'line' ,'p1':50005 ,'p2':30005})
    appendLine({'t':'line' ,'p1':30005 ,'p2':20005})
    appendLine({'t':'line' ,'p1':20005 ,'p2':10005})
    appendLine({'t':'line' ,'p1':10005 ,'p2':4})
    appendLine({'t':'line' ,'p1':60000 ,'p2':80000})
    appendLine({'t':'line' ,'p1':80000 ,'p2':90000})
    appendLine({'t':'line' ,'p1':90000 ,'p2':100000})
    appendLine({'t':'line' ,'p1':60000 ,'p2':60002})
    appendLine({'t':'line' ,'p1':60002 ,'p2':60003})
    appendLine({'t':'line' ,'p1':60003 ,'p2':60004})
    appendLine({'t':'line' ,'p1':60004 ,'p2':100004})
    appendLine({'t':'line' ,'p1':100004 ,'p2':100000})
    appendLine({'t':'line' ,'p1':100000 ,'p2':110001})
    appendLine({'t':'line' ,'p1':110001 ,'p2':110002})
    appendLine({'t':'line' ,'p1':110002 ,'p2':110003})
    appendLine({'t':'line' ,'p1':110003 ,'p2':110005})
    appendLine({'t':'line' ,'p1':110005 ,'p2':100004})
    appendLine({'t':'line' ,'p1':110005 ,'p2':90005})
    appendLine({'t':'line' ,'p1':90005 ,'p2':80005})
    appendLine({'t':'line' ,'p1':80005 ,'p2':70005})
    appendLine({'t':'line' ,'p1':70005 ,'p2':60004})
    appendLine({'t':'line' ,'p1':6 ,'p2':20006})
    appendLine({'t':'line' ,'p1':20006 ,'p2':30006})
    appendLine({'t':'line' ,'p1':30006 ,'p2':40006})
    appendLine({'t':'line' ,'p1':6 ,'p2':8})
    appendLine({'t':'line' ,'p1':8 ,'p2':9})
    appendLine({'t':'line' ,'p1':9 ,'p2':10})
    appendLine({'t':'line' ,'p1':10 ,'p2':40010})
    appendLine({'t':'line' ,'p1':40010 ,'p2':40006})
    appendLine({'t':'line' ,'p1':40006 ,'p2':50007})
    appendLine({'t':'line' ,'p1':50007 ,'p2':50008})
    appendLine({'t':'line' ,'p1':50008 ,'p2':50009})
    appendLine({'t':'line' ,'p1':50009 ,'p2':50011})
    appendLine({'t':'line' ,'p1':50011 ,'p2':40010})
    appendLine({'t':'line' ,'p1':50011 ,'p2':30011})
    appendLine({'t':'line' ,'p1':30011 ,'p2':20011})
    appendLine({'t':'line' ,'p1':20011 ,'p2':10011})
    appendLine({'t':'line' ,'p1':10011 ,'p2':10})
    appendLine({'t':'line' ,'p1':60006 ,'p2':80006})
    appendLine({'t':'line' ,'p1':80006 ,'p2':90006})
    appendLine({'t':'line' ,'p1':90006 ,'p2':100006})
    appendLine({'t':'line' ,'p1':60006 ,'p2':60008})
    appendLine({'t':'line' ,'p1':60008 ,'p2':60009})
    appendLine({'t':'line' ,'p1':60009 ,'p2':60010})
    appendLine({'t':'line' ,'p1':60010 ,'p2':100010})
    appendLine({'t':'line' ,'p1':100010 ,'p2':100006})
    appendLine({'t':'line' ,'p1':100006 ,'p2':110007})
    appendLine({'t':'line' ,'p1':110007 ,'p2':110008})
    appendLine({'t':'line' ,'p1':110008 ,'p2':110009})
    appendLine({'t':'line' ,'p1':110009 ,'p2':110011})
    appendLine({'t':'line' ,'p1':110011 ,'p2':100010})
    appendLine({'t':'line' ,'p1':110011 ,'p2':90011})
    appendLine({'t':'line' ,'p1':90011 ,'p2':80011})
    appendLine({'t':'line' ,'p1':80011 ,'p2':70011})
    appendLine({'t':'line' ,'p1':70011 ,'p2':60010})
    appendLine({'t':'line' ,'p1':170003 ,'p2':180003})
    appendLine({'t':'line' ,'p1':150005 ,'p2':150006})
    appendLine({'t':'line' ,'p1':200005 ,'p2':200006})
    appendLine({'t':'line' ,'p1':170008 ,'p2':180008})
    appendLine({'t':'line' ,'p1':120000 ,'p2':140000})
    appendLine({'t':'line' ,'p1':140000 ,'p2':150000})
    appendLine({'t':'line' ,'p1':150000 ,'p2':160000})
    appendLine({'t':'line' ,'p1':120000 ,'p2':120002})
    appendLine({'t':'line' ,'p1':120002 ,'p2':120003})
    appendLine({'t':'line' ,'p1':120003 ,'p2':120004})
    appendLine({'t':'line' ,'p1':120004 ,'p2':160004})
    appendLine({'t':'line' ,'p1':160004 ,'p2':160000})
    appendLine({'t':'line' ,'p1':160000 ,'p2':170001})
    appendLine({'t':'line' ,'p1':170001 ,'p2':170002})
    appendLine({'t':'line' ,'p1':170002 ,'p2':170003})
    appendLine({'t':'line' ,'p1':170003 ,'p2':170005})
    appendLine({'t':'line' ,'p1':170005 ,'p2':160004})
    appendLine({'t':'line' ,'p1':170005 ,'p2':150005})
    appendLine({'t':'line' ,'p1':150005 ,'p2':140005})
    appendLine({'t':'line' ,'p1':140005 ,'p2':130005})
    appendLine({'t':'line' ,'p1':130005 ,'p2':120004})
    appendLine({'t':'line' ,'p1':180000 ,'p2':200000})
    appendLine({'t':'line' ,'p1':200000 ,'p2':210000})
    appendLine({'t':'line' ,'p1':210000 ,'p2':220000})
    appendLine({'t':'line' ,'p1':180000 ,'p2':180002})
    appendLine({'t':'line' ,'p1':180002 ,'p2':180003})
    appendLine({'t':'line' ,'p1':180003 ,'p2':180004})
    appendLine({'t':'line' ,'p1':180004 ,'p2':220004})
    appendLine({'t':'line' ,'p1':220004 ,'p2':220000})
    appendLine({'t':'line' ,'p1':220000 ,'p2':230001})
    appendLine({'t':'line' ,'p1':230001 ,'p2':230002})
    appendLine({'t':'line' ,'p1':230002 ,'p2':230003})
    appendLine({'t':'line' ,'p1':230003 ,'p2':230005})
    appendLine({'t':'line' ,'p1':230005 ,'p2':220004})
    appendLine({'t':'line' ,'p1':230005 ,'p2':210005})
    appendLine({'t':'line' ,'p1':210005 ,'p2':200005})
    appendLine({'t':'line' ,'p1':200005 ,'p2':190005})
    appendLine({'t':'line' ,'p1':190005 ,'p2':180004})
    appendLine({'t':'line' ,'p1':120006 ,'p2':140006})
    appendLine({'t':'line' ,'p1':140006 ,'p2':150006})
    appendLine({'t':'line' ,'p1':150006 ,'p2':160006})
    appendLine({'t':'line' ,'p1':120006 ,'p2':120008})
    appendLine({'t':'line' ,'p1':120008 ,'p2':120009})
    appendLine({'t':'line' ,'p1':120009 ,'p2':120010})
    appendLine({'t':'line' ,'p1':120010 ,'p2':160010})
    appendLine({'t':'line' ,'p1':160010 ,'p2':160006})
    appendLine({'t':'line' ,'p1':160006 ,'p2':170007})
    appendLine({'t':'line' ,'p1':170007 ,'p2':170008})
    appendLine({'t':'line' ,'p1':170008 ,'p2':170009})
    appendLine({'t':'line' ,'p1':170009 ,'p2':170011})
    appendLine({'t':'line' ,'p1':170011 ,'p2':160010})
    appendLine({'t':'line' ,'p1':170011 ,'p2':150011})
    appendLine({'t':'line' ,'p1':150011 ,'p2':140011})
    appendLine({'t':'line' ,'p1':140011 ,'p2':130011})
    appendLine({'t':'line' ,'p1':130011 ,'p2':120010})
    appendLine({'t':'line' ,'p1':180006 ,'p2':200006})
    appendLine({'t':'line' ,'p1':200006 ,'p2':210006})
    appendLine({'t':'line' ,'p1':210006 ,'p2':220006})
    appendLine({'t':'line' ,'p1':180006 ,'p2':180008})
    appendLine({'t':'line' ,'p1':180008 ,'p2':180009})
    appendLine({'t':'line' ,'p1':180009 ,'p2':180010})
    appendLine({'t':'line' ,'p1':180010 ,'p2':220010})
    appendLine({'t':'line' ,'p1':220010 ,'p2':220006})
    appendLine({'t':'line' ,'p1':220006 ,'p2':230007})
    appendLine({'t':'line' ,'p1':230007 ,'p2':230008})
    appendLine({'t':'line' ,'p1':230008 ,'p2':230009})
    appendLine({'t':'line' ,'p1':230009 ,'p2':230011})
    appendLine({'t':'line' ,'p1':230011 ,'p2':220010})
    appendLine({'t':'line' ,'p1':230011 ,'p2':210011})
    appendLine({'t':'line' ,'p1':210011 ,'p2':200011})
    appendLine({'t':'line' ,'p1':200011 ,'p2':190011})
    appendLine({'t':'line' ,'p1':190011 ,'p2':180010})
    appendLine({'t':'line' ,'p1':110003 ,'p2':120003})
    appendLine({'t':'line' ,'p1':90005 ,'p2':90006})
    appendLine({'t':'line' ,'p1':140005 ,'p2':140006})
    appendLine({'t':'line' ,'p1':110008 ,'p2':120008})
    appendLine({'t':'line' ,'p1':290003 ,'p2':300003})
    appendLine({'t':'line' ,'p1':270005 ,'p2':270006})
    appendLine({'t':'line' ,'p1':320005 ,'p2':320006})
    appendLine({'t':'line' ,'p1':290008 ,'p2':300008})
    appendLine({'t':'line' ,'p1':240000 ,'p2':260000})
    appendLine({'t':'line' ,'p1':260000 ,'p2':270000})
    appendLine({'t':'line' ,'p1':270000 ,'p2':280000})
    appendLine({'t':'line' ,'p1':240000 ,'p2':240002})
    appendLine({'t':'line' ,'p1':240002 ,'p2':240003})
    appendLine({'t':'line' ,'p1':240003 ,'p2':240004})
    appendLine({'t':'line' ,'p1':240004 ,'p2':280004})
    appendLine({'t':'line' ,'p1':280004 ,'p2':280000})
    appendLine({'t':'line' ,'p1':280000 ,'p2':290001})
    appendLine({'t':'line' ,'p1':290001 ,'p2':290002})
    appendLine({'t':'line' ,'p1':290002 ,'p2':290003})
    appendLine({'t':'line' ,'p1':290003 ,'p2':290005})
    appendLine({'t':'line' ,'p1':290005 ,'p2':280004})
    appendLine({'t':'line' ,'p1':290005 ,'p2':270005})
    appendLine({'t':'line' ,'p1':270005 ,'p2':260005})
    appendLine({'t':'line' ,'p1':260005 ,'p2':250005})
    appendLine({'t':'line' ,'p1':250005 ,'p2':240004})
    appendLine({'t':'line' ,'p1':300000 ,'p2':320000})
    appendLine({'t':'line' ,'p1':320000 ,'p2':330000})
    appendLine({'t':'line' ,'p1':330000 ,'p2':340000})
    appendLine({'t':'line' ,'p1':300000 ,'p2':300002})
    appendLine({'t':'line' ,'p1':300002 ,'p2':300003})
    appendLine({'t':'line' ,'p1':300003 ,'p2':300004})
    appendLine({'t':'line' ,'p1':300004 ,'p2':340004})
    appendLine({'t':'line' ,'p1':340004 ,'p2':340000})
    appendLine({'t':'line' ,'p1':340000 ,'p2':350001})
    appendLine({'t':'line' ,'p1':350001 ,'p2':350002})
    appendLine({'t':'line' ,'p1':350002 ,'p2':350003})
    appendLine({'t':'line' ,'p1':350003 ,'p2':350005})
    appendLine({'t':'line' ,'p1':350005 ,'p2':340004})
    appendLine({'t':'line' ,'p1':350005 ,'p2':330005})
    appendLine({'t':'line' ,'p1':330005 ,'p2':320005})
    appendLine({'t':'line' ,'p1':320005 ,'p2':310005})
    appendLine({'t':'line' ,'p1':310005 ,'p2':300004})
    appendLine({'t':'line' ,'p1':240006 ,'p2':260006})
    appendLine({'t':'line' ,'p1':260006 ,'p2':270006})
    appendLine({'t':'line' ,'p1':270006 ,'p2':280006})
    appendLine({'t':'line' ,'p1':240006 ,'p2':240008})
    appendLine({'t':'line' ,'p1':240008 ,'p2':240009})
    appendLine({'t':'line' ,'p1':240009 ,'p2':240010})
    appendLine({'t':'line' ,'p1':240010 ,'p2':280010})
    appendLine({'t':'line' ,'p1':280010 ,'p2':280006})
    appendLine({'t':'line' ,'p1':280006 ,'p2':290007})
    appendLine({'t':'line' ,'p1':290007 ,'p2':290008})
    appendLine({'t':'line' ,'p1':290008 ,'p2':290009})
    appendLine({'t':'line' ,'p1':290009 ,'p2':290011})
    appendLine({'t':'line' ,'p1':290011 ,'p2':280010})
    appendLine({'t':'line' ,'p1':290011 ,'p2':270011})
    appendLine({'t':'line' ,'p1':270011 ,'p2':260011})
    appendLine({'t':'line' ,'p1':260011 ,'p2':250011})
    appendLine({'t':'line' ,'p1':250011 ,'p2':240010})
    appendLine({'t':'line' ,'p1':300006 ,'p2':320006})
    appendLine({'t':'line' ,'p1':320006 ,'p2':330006})
    appendLine({'t':'line' ,'p1':330006 ,'p2':340006})
    appendLine({'t':'line' ,'p1':300006 ,'p2':300008})
    appendLine({'t':'line' ,'p1':300008 ,'p2':300009})
    appendLine({'t':'line' ,'p1':300009 ,'p2':300010})
    appendLine({'t':'line' ,'p1':300010 ,'p2':340010})
    appendLine({'t':'line' ,'p1':340010 ,'p2':340006})
    appendLine({'t':'line' ,'p1':340006 ,'p2':350007})
    appendLine({'t':'line' ,'p1':350007 ,'p2':350008})
    appendLine({'t':'line' ,'p1':350008 ,'p2':350009})
    appendLine({'t':'line' ,'p1':350009 ,'p2':350011})
    appendLine({'t':'line' ,'p1':350011 ,'p2':340010})
    appendLine({'t':'line' ,'p1':350011 ,'p2':330011})
    appendLine({'t':'line' ,'p1':330011 ,'p2':320011})
    appendLine({'t':'line' ,'p1':320011 ,'p2':310011})
    appendLine({'t':'line' ,'p1':310011 ,'p2':300010})
    appendLine({'t':'line' ,'p1':230003 ,'p2':240003})
    appendLine({'t':'line' ,'p1':210005 ,'p2':210006})
    appendLine({'t':'line' ,'p1':260005 ,'p2':260006})
    appendLine({'t':'line' ,'p1':230008 ,'p2':240008})
    appendLine({'t':'line' ,'p1':410003 ,'p2':420003})
    appendLine({'t':'line' ,'p1':390005 ,'p2':390006})
    appendLine({'t':'line' ,'p1':440005 ,'p2':440006})
    appendLine({'t':'line' ,'p1':410008 ,'p2':420008})
    appendLine({'t':'line' ,'p1':360000 ,'p2':380000})
    appendLine({'t':'line' ,'p1':380000 ,'p2':390000})
    appendLine({'t':'line' ,'p1':390000 ,'p2':400000})
    appendLine({'t':'line' ,'p1':360000 ,'p2':360002})
    appendLine({'t':'line' ,'p1':360002 ,'p2':360003})
    appendLine({'t':'line' ,'p1':360003 ,'p2':360004})
    appendLine({'t':'line' ,'p1':360004 ,'p2':400004})
    appendLine({'t':'line' ,'p1':400004 ,'p2':400000})
    appendLine({'t':'line' ,'p1':400000 ,'p2':410001})
    appendLine({'t':'line' ,'p1':410001 ,'p2':410002})
    appendLine({'t':'line' ,'p1':410002 ,'p2':410003})
    appendLine({'t':'line' ,'p1':410003 ,'p2':410005})
    appendLine({'t':'line' ,'p1':410005 ,'p2':400004})
    appendLine({'t':'line' ,'p1':410005 ,'p2':390005})
    appendLine({'t':'line' ,'p1':390005 ,'p2':380005})
    appendLine({'t':'line' ,'p1':380005 ,'p2':370005})
    appendLine({'t':'line' ,'p1':370005 ,'p2':360004})
    appendLine({'t':'line' ,'p1':420000 ,'p2':440000})
    appendLine({'t':'line' ,'p1':440000 ,'p2':450000})
    appendLine({'t':'line' ,'p1':450000 ,'p2':460000})
    appendLine({'t':'line' ,'p1':420000 ,'p2':420002})
    appendLine({'t':'line' ,'p1':420002 ,'p2':420003})
    appendLine({'t':'line' ,'p1':420003 ,'p2':420004})
    appendLine({'t':'line' ,'p1':420004 ,'p2':460004})
    appendLine({'t':'line' ,'p1':460004 ,'p2':460000})
    appendLine({'t':'line' ,'p1':460000 ,'p2':470001})
    appendLine({'t':'line' ,'p1':470001 ,'p2':470002})
    appendLine({'t':'line' ,'p1':470002 ,'p2':470003})
    appendLine({'t':'line' ,'p1':470003 ,'p2':470005})
    appendLine({'t':'line' ,'p1':470005 ,'p2':460004})
    appendLine({'t':'line' ,'p1':470005 ,'p2':450005})
    appendLine({'t':'line' ,'p1':450005 ,'p2':440005})
    appendLine({'t':'line' ,'p1':440005 ,'p2':430005})
    appendLine({'t':'line' ,'p1':430005 ,'p2':420004})
    appendLine({'t':'line' ,'p1':360006 ,'p2':380006})
    appendLine({'t':'line' ,'p1':380006 ,'p2':390006})
    appendLine({'t':'line' ,'p1':390006 ,'p2':400006})
    appendLine({'t':'line' ,'p1':360006 ,'p2':360008})
    appendLine({'t':'line' ,'p1':360008 ,'p2':360009})
    appendLine({'t':'line' ,'p1':360009 ,'p2':360010})
    appendLine({'t':'line' ,'p1':360010 ,'p2':400010})
    appendLine({'t':'line' ,'p1':400010 ,'p2':400006})
    appendLine({'t':'line' ,'p1':400006 ,'p2':410007})
    appendLine({'t':'line' ,'p1':410007 ,'p2':410008})
    appendLine({'t':'line' ,'p1':410008 ,'p2':410009})
    appendLine({'t':'line' ,'p1':410009 ,'p2':410011})
    appendLine({'t':'line' ,'p1':410011 ,'p2':400010})
    appendLine({'t':'line' ,'p1':410011 ,'p2':390011})
    appendLine({'t':'line' ,'p1':390011 ,'p2':380011})
    appendLine({'t':'line' ,'p1':380011 ,'p2':370011})
    appendLine({'t':'line' ,'p1':370011 ,'p2':360010})
    appendLine({'t':'line' ,'p1':420006 ,'p2':440006})
    appendLine({'t':'line' ,'p1':440006 ,'p2':450006})
    appendLine({'t':'line' ,'p1':450006 ,'p2':460006})
    appendLine({'t':'line' ,'p1':420006 ,'p2':420008})
    appendLine({'t':'line' ,'p1':420008 ,'p2':420009})
    appendLine({'t':'line' ,'p1':420009 ,'p2':420010})
    appendLine({'t':'line' ,'p1':420010 ,'p2':460010})
    appendLine({'t':'line' ,'p1':460010 ,'p2':460006})
    appendLine({'t':'line' ,'p1':460006 ,'p2':470007})
    appendLine({'t':'line' ,'p1':470007 ,'p2':470008})
    appendLine({'t':'line' ,'p1':470008 ,'p2':470009})
    appendLine({'t':'line' ,'p1':470009 ,'p2':470011})
    appendLine({'t':'line' ,'p1':470011 ,'p2':460010})
    appendLine({'t':'line' ,'p1':470011 ,'p2':450011})
    appendLine({'t':'line' ,'p1':450011 ,'p2':440011})
    appendLine({'t':'line' ,'p1':440011 ,'p2':430011})
    appendLine({'t':'line' ,'p1':430011 ,'p2':420010})
    appendLine({'t':'line' ,'p1':350003 ,'p2':360003})
    appendLine({'t':'line' ,'p1':330005 ,'p2':330006})
    appendLine({'t':'line' ,'p1':380005 ,'p2':380006})
    appendLine({'t':'line' ,'p1':350008 ,'p2':360008})
    appendLine({'t':'line' ,'p1':530003 ,'p2':540003})
    appendLine({'t':'line' ,'p1':510005 ,'p2':510006})
    appendLine({'t':'line' ,'p1':560005 ,'p2':560006})
    appendLine({'t':'line' ,'p1':530008 ,'p2':540008})
    appendLine({'t':'line' ,'p1':480000 ,'p2':500000})
    appendLine({'t':'line' ,'p1':500000 ,'p2':510000})
    appendLine({'t':'line' ,'p1':510000 ,'p2':520000})
    appendLine({'t':'line' ,'p1':480000 ,'p2':480002})
    appendLine({'t':'line' ,'p1':480002 ,'p2':480003})
    appendLine({'t':'line' ,'p1':480003 ,'p2':480004})
    appendLine({'t':'line' ,'p1':480004 ,'p2':520004})
    appendLine({'t':'line' ,'p1':520004 ,'p2':520000})
    appendLine({'t':'line' ,'p1':520000 ,'p2':530001})
    appendLine({'t':'line' ,'p1':530001 ,'p2':530002})
    appendLine({'t':'line' ,'p1':530002 ,'p2':530003})
    appendLine({'t':'line' ,'p1':530003 ,'p2':530005})
    appendLine({'t':'line' ,'p1':530005 ,'p2':520004})
    appendLine({'t':'line' ,'p1':530005 ,'p2':510005})
    appendLine({'t':'line' ,'p1':510005 ,'p2':500005})
    appendLine({'t':'line' ,'p1':500005 ,'p2':490005})
    appendLine({'t':'line' ,'p1':490005 ,'p2':480004})
    appendLine({'t':'line' ,'p1':540000 ,'p2':560000})
    appendLine({'t':'line' ,'p1':560000 ,'p2':570000})
    appendLine({'t':'line' ,'p1':570000 ,'p2':580000})
    appendLine({'t':'line' ,'p1':540000 ,'p2':540002})
    appendLine({'t':'line' ,'p1':540002 ,'p2':540003})
    appendLine({'t':'line' ,'p1':540003 ,'p2':540004})
    appendLine({'t':'line' ,'p1':540004 ,'p2':580004})
    appendLine({'t':'line' ,'p1':580004 ,'p2':580000})
    appendLine({'t':'line' ,'p1':580000 ,'p2':590001})
    appendLine({'t':'line' ,'p1':590001 ,'p2':590002})
    appendLine({'t':'line' ,'p1':590002 ,'p2':590003})
    appendLine({'t':'line' ,'p1':590003 ,'p2':590005})
    appendLine({'t':'line' ,'p1':590005 ,'p2':580004})
    appendLine({'t':'line' ,'p1':590005 ,'p2':570005})
    appendLine({'t':'line' ,'p1':570005 ,'p2':560005})
    appendLine({'t':'line' ,'p1':560005 ,'p2':550005})
    appendLine({'t':'line' ,'p1':550005 ,'p2':540004})
    appendLine({'t':'line' ,'p1':480006 ,'p2':500006})
    appendLine({'t':'line' ,'p1':500006 ,'p2':510006})
    appendLine({'t':'line' ,'p1':510006 ,'p2':520006})
    appendLine({'t':'line' ,'p1':480006 ,'p2':480008})
    appendLine({'t':'line' ,'p1':480008 ,'p2':480009})
    appendLine({'t':'line' ,'p1':480009 ,'p2':480010})
    appendLine({'t':'line' ,'p1':480010 ,'p2':520010})
    appendLine({'t':'line' ,'p1':520010 ,'p2':520006})
    appendLine({'t':'line' ,'p1':520006 ,'p2':530007})
    appendLine({'t':'line' ,'p1':530007 ,'p2':530008})
    appendLine({'t':'line' ,'p1':530008 ,'p2':530009})
    appendLine({'t':'line' ,'p1':530009 ,'p2':530011})
    appendLine({'t':'line' ,'p1':530011 ,'p2':520010})
    appendLine({'t':'line' ,'p1':530011 ,'p2':510011})
    appendLine({'t':'line' ,'p1':510011 ,'p2':500011})
    appendLine({'t':'line' ,'p1':500011 ,'p2':490011})
    appendLine({'t':'line' ,'p1':490011 ,'p2':480010})
    appendLine({'t':'line' ,'p1':540006 ,'p2':560006})
    appendLine({'t':'line' ,'p1':560006 ,'p2':570006})
    appendLine({'t':'line' ,'p1':570006 ,'p2':580006})
    appendLine({'t':'line' ,'p1':540006 ,'p2':540008})
    appendLine({'t':'line' ,'p1':540008 ,'p2':540009})
    appendLine({'t':'line' ,'p1':540009 ,'p2':540010})
    appendLine({'t':'line' ,'p1':540010 ,'p2':580010})
    appendLine({'t':'line' ,'p1':580010 ,'p2':580006})
    appendLine({'t':'line' ,'p1':580006 ,'p2':590007})
    appendLine({'t':'line' ,'p1':590007 ,'p2':590008})
    appendLine({'t':'line' ,'p1':590008 ,'p2':590009})
    appendLine({'t':'line' ,'p1':590009 ,'p2':590011})
    appendLine({'t':'line' ,'p1':590011 ,'p2':580010})
    appendLine({'t':'line' ,'p1':590011 ,'p2':570011})
    appendLine({'t':'line' ,'p1':570011 ,'p2':560011})
    appendLine({'t':'line' ,'p1':560011 ,'p2':550011})
    appendLine({'t':'line' ,'p1':550011 ,'p2':540010})
    appendLine({'t':'line' ,'p1':470003 ,'p2':480003})
    appendLine({'t':'line' ,'p1':450005 ,'p2':450006})
    appendLine({'t':'line' ,'p1':500005 ,'p2':500006})
    appendLine({'t':'line' ,'p1':470008 ,'p2':480008})
    appendLine({'t':'line' ,'p1':650003 ,'p2':660003})
    appendLine({'t':'line' ,'p1':630005 ,'p2':630006})
    appendLine({'t':'line' ,'p1':680005 ,'p2':680006})
    appendLine({'t':'line' ,'p1':650008 ,'p2':660008})
    appendLine({'t':'line' ,'p1':600000 ,'p2':620000})
    appendLine({'t':'line' ,'p1':620000 ,'p2':630000})
    appendLine({'t':'line' ,'p1':630000 ,'p2':640000})
    appendLine({'t':'line' ,'p1':600000 ,'p2':600002})
    appendLine({'t':'line' ,'p1':600002 ,'p2':600003})
    appendLine({'t':'line' ,'p1':600003 ,'p2':600004})
    appendLine({'t':'line' ,'p1':600004 ,'p2':640004})
    appendLine({'t':'line' ,'p1':640004 ,'p2':640000})
    appendLine({'t':'line' ,'p1':640000 ,'p2':650001})
    appendLine({'t':'line' ,'p1':650001 ,'p2':650002})
    appendLine({'t':'line' ,'p1':650002 ,'p2':650003})
    appendLine({'t':'line' ,'p1':650003 ,'p2':650005})
    appendLine({'t':'line' ,'p1':650005 ,'p2':640004})
    appendLine({'t':'line' ,'p1':650005 ,'p2':630005})
    appendLine({'t':'line' ,'p1':630005 ,'p2':620005})
    appendLine({'t':'line' ,'p1':620005 ,'p2':610005})
    appendLine({'t':'line' ,'p1':610005 ,'p2':600004})
    appendLine({'t':'line' ,'p1':660000 ,'p2':680000})
    appendLine({'t':'line' ,'p1':680000 ,'p2':690000})
    appendLine({'t':'line' ,'p1':690000 ,'p2':700000})
    appendLine({'t':'line' ,'p1':660000 ,'p2':660002})
    appendLine({'t':'line' ,'p1':660002 ,'p2':660003})
    appendLine({'t':'line' ,'p1':660003 ,'p2':660004})
    appendLine({'t':'line' ,'p1':660004 ,'p2':700004})
    appendLine({'t':'line' ,'p1':700004 ,'p2':700000})
    appendLine({'t':'line' ,'p1':700000 ,'p2':710001})
    appendLine({'t':'line' ,'p1':710001 ,'p2':710002})
    appendLine({'t':'line' ,'p1':710002 ,'p2':710003})
    appendLine({'t':'line' ,'p1':710003 ,'p2':710005})
    appendLine({'t':'line' ,'p1':710005 ,'p2':700004})
    appendLine({'t':'line' ,'p1':710005 ,'p2':690005})
    appendLine({'t':'line' ,'p1':690005 ,'p2':680005})
    appendLine({'t':'line' ,'p1':680005 ,'p2':670005})
    appendLine({'t':'line' ,'p1':670005 ,'p2':660004})
    appendLine({'t':'line' ,'p1':600006 ,'p2':620006})
    appendLine({'t':'line' ,'p1':620006 ,'p2':630006})
    appendLine({'t':'line' ,'p1':630006 ,'p2':640006})
    appendLine({'t':'line' ,'p1':600006 ,'p2':600008})
    appendLine({'t':'line' ,'p1':600008 ,'p2':600009})
    appendLine({'t':'line' ,'p1':600009 ,'p2':600010})
    appendLine({'t':'line' ,'p1':600010 ,'p2':640010})
    appendLine({'t':'line' ,'p1':640010 ,'p2':640006})
    appendLine({'t':'line' ,'p1':640006 ,'p2':650007})
    appendLine({'t':'line' ,'p1':650007 ,'p2':650008})
    appendLine({'t':'line' ,'p1':650008 ,'p2':650009})
    appendLine({'t':'line' ,'p1':650009 ,'p2':650011})
    appendLine({'t':'line' ,'p1':650011 ,'p2':640010})
    appendLine({'t':'line' ,'p1':650011 ,'p2':630011})
    appendLine({'t':'line' ,'p1':630011 ,'p2':620011})
    appendLine({'t':'line' ,'p1':620011 ,'p2':610011})
    appendLine({'t':'line' ,'p1':610011 ,'p2':600010})
    appendLine({'t':'line' ,'p1':660006 ,'p2':680006})
    appendLine({'t':'line' ,'p1':680006 ,'p2':690006})
    appendLine({'t':'line' ,'p1':690006 ,'p2':700006})
    appendLine({'t':'line' ,'p1':660006 ,'p2':660008})
    appendLine({'t':'line' ,'p1':660008 ,'p2':660009})
    appendLine({'t':'line' ,'p1':660009 ,'p2':660010})
    appendLine({'t':'line' ,'p1':660010 ,'p2':700010})
    appendLine({'t':'line' ,'p1':700010 ,'p2':700006})
    appendLine({'t':'line' ,'p1':700006 ,'p2':710007})
    appendLine({'t':'line' ,'p1':710007 ,'p2':710008})
    appendLine({'t':'line' ,'p1':710008 ,'p2':710009})
    appendLine({'t':'line' ,'p1':710009 ,'p2':710011})
    appendLine({'t':'line' ,'p1':710011 ,'p2':700010})
    appendLine({'t':'line' ,'p1':710011 ,'p2':690011})
    appendLine({'t':'line' ,'p1':690011 ,'p2':680011})
    appendLine({'t':'line' ,'p1':680011 ,'p2':670011})
    appendLine({'t':'line' ,'p1':670011 ,'p2':660010})
    appendLine({'t':'line' ,'p1':590003 ,'p2':600003})
    appendLine({'t':'line' ,'p1':570005 ,'p2':570006})
    appendLine({'t':'line' ,'p1':620005 ,'p2':620006})
    appendLine({'t':'line' ,'p1':590008 ,'p2':600008})
    appendLine({'t':'line' ,'p1':770003 ,'p2':780003})
    appendLine({'t':'line' ,'p1':750005 ,'p2':750006})
    appendLine({'t':'line' ,'p1':800005 ,'p2':800006})
    appendLine({'t':'line' ,'p1':770008 ,'p2':780008})
    appendLine({'t':'line' ,'p1':720000 ,'p2':740000})
    appendLine({'t':'line' ,'p1':740000 ,'p2':750000})
    appendLine({'t':'line' ,'p1':750000 ,'p2':760000})
    appendLine({'t':'line' ,'p1':720000 ,'p2':720002})
    appendLine({'t':'line' ,'p1':720002 ,'p2':720003})
    appendLine({'t':'line' ,'p1':720003 ,'p2':720004})
    appendLine({'t':'line' ,'p1':720004 ,'p2':760004})
    appendLine({'t':'line' ,'p1':760004 ,'p2':760000})
    appendLine({'t':'line' ,'p1':760000 ,'p2':770001})
    appendLine({'t':'line' ,'p1':770001 ,'p2':770002})
    appendLine({'t':'line' ,'p1':770002 ,'p2':770003})
    appendLine({'t':'line' ,'p1':770003 ,'p2':770005})
    appendLine({'t':'line' ,'p1':770005 ,'p2':760004})
    appendLine({'t':'line' ,'p1':770005 ,'p2':750005})
    appendLine({'t':'line' ,'p1':750005 ,'p2':740005})
    appendLine({'t':'line' ,'p1':740005 ,'p2':730005})
    appendLine({'t':'line' ,'p1':730005 ,'p2':720004})
    appendLine({'t':'line' ,'p1':780000 ,'p2':800000})
    appendLine({'t':'line' ,'p1':800000 ,'p2':810000})
    appendLine({'t':'line' ,'p1':810000 ,'p2':820000})
    appendLine({'t':'line' ,'p1':780000 ,'p2':780002})
    appendLine({'t':'line' ,'p1':780002 ,'p2':780003})
    appendLine({'t':'line' ,'p1':780003 ,'p2':780004})
    appendLine({'t':'line' ,'p1':780004 ,'p2':820004})
    appendLine({'t':'line' ,'p1':820004 ,'p2':820000})
    appendLine({'t':'line' ,'p1':820000 ,'p2':830001})
    appendLine({'t':'line' ,'p1':830001 ,'p2':830002})
    appendLine({'t':'line' ,'p1':830002 ,'p2':830003})
    appendLine({'t':'line' ,'p1':830003 ,'p2':830005})
    appendLine({'t':'line' ,'p1':830005 ,'p2':820004})
    appendLine({'t':'line' ,'p1':830005 ,'p2':810005})
    appendLine({'t':'line' ,'p1':810005 ,'p2':800005})
    appendLine({'t':'line' ,'p1':800005 ,'p2':790005})
    appendLine({'t':'line' ,'p1':790005 ,'p2':780004})
    appendLine({'t':'line' ,'p1':720006 ,'p2':740006})
    appendLine({'t':'line' ,'p1':740006 ,'p2':750006})
    appendLine({'t':'line' ,'p1':750006 ,'p2':760006})
    appendLine({'t':'line' ,'p1':720006 ,'p2':720008})
    appendLine({'t':'line' ,'p1':720008 ,'p2':720009})
    appendLine({'t':'line' ,'p1':720009 ,'p2':720010})
    appendLine({'t':'line' ,'p1':720010 ,'p2':760010})
    appendLine({'t':'line' ,'p1':760010 ,'p2':760006})
    appendLine({'t':'line' ,'p1':760006 ,'p2':770007})
    appendLine({'t':'line' ,'p1':770007 ,'p2':770008})
    appendLine({'t':'line' ,'p1':770008 ,'p2':770009})
    appendLine({'t':'line' ,'p1':770009 ,'p2':770011})
    appendLine({'t':'line' ,'p1':770011 ,'p2':760010})
    appendLine({'t':'line' ,'p1':770011 ,'p2':750011})
    appendLine({'t':'line' ,'p1':750011 ,'p2':740011})
    appendLine({'t':'line' ,'p1':740011 ,'p2':730011})
    appendLine({'t':'line' ,'p1':730011 ,'p2':720010})
    appendLine({'t':'line' ,'p1':780006 ,'p2':800006})
    appendLine({'t':'line' ,'p1':800006 ,'p2':810006})
    appendLine({'t':'line' ,'p1':810006 ,'p2':820006})
    appendLine({'t':'line' ,'p1':780006 ,'p2':780008})
    appendLine({'t':'line' ,'p1':780008 ,'p2':780009})
    appendLine({'t':'line' ,'p1':780009 ,'p2':780010})
    appendLine({'t':'line' ,'p1':780010 ,'p2':820010})
    appendLine({'t':'line' ,'p1':820010 ,'p2':820006})
    appendLine({'t':'line' ,'p1':820006 ,'p2':830007})
    appendLine({'t':'line' ,'p1':830007 ,'p2':830008})
    appendLine({'t':'line' ,'p1':830008 ,'p2':830009})
    appendLine({'t':'line' ,'p1':830009 ,'p2':830011})
    appendLine({'t':'line' ,'p1':830011 ,'p2':820010})
    appendLine({'t':'line' ,'p1':830011 ,'p2':810011})
    appendLine({'t':'line' ,'p1':810011 ,'p2':800011})
    appendLine({'t':'line' ,'p1':800011 ,'p2':790011})
    appendLine({'t':'line' ,'p1':790011 ,'p2':780010})
    appendLine({'t':'line' ,'p1':710003 ,'p2':720003})
    appendLine({'t':'line' ,'p1':690005 ,'p2':690006})
    appendLine({'t':'line' ,'p1':740005 ,'p2':740006})
    appendLine({'t':'line' ,'p1':710008 ,'p2':720008})
    appendLine({'t':'line' ,'p1':890003 ,'p2':900003})
    appendLine({'t':'line' ,'p1':870005 ,'p2':870006})
    appendLine({'t':'line' ,'p1':920005 ,'p2':920006})
    appendLine({'t':'line' ,'p1':890008 ,'p2':900008})
    appendLine({'t':'line' ,'p1':840000 ,'p2':860000})
    appendLine({'t':'line' ,'p1':860000 ,'p2':870000})
    appendLine({'t':'line' ,'p1':870000 ,'p2':880000})
    appendLine({'t':'line' ,'p1':840000 ,'p2':840002})
    appendLine({'t':'line' ,'p1':840002 ,'p2':840003})
    appendLine({'t':'line' ,'p1':840003 ,'p2':840004})
    appendLine({'t':'line' ,'p1':840004 ,'p2':880004})
    appendLine({'t':'line' ,'p1':880004 ,'p2':880000})
    appendLine({'t':'line' ,'p1':880000 ,'p2':890001})
    appendLine({'t':'line' ,'p1':890001 ,'p2':890002})
    appendLine({'t':'line' ,'p1':890002 ,'p2':890003})
    appendLine({'t':'line' ,'p1':890003 ,'p2':890005})
    appendLine({'t':'line' ,'p1':890005 ,'p2':880004})
    appendLine({'t':'line' ,'p1':890005 ,'p2':870005})
    appendLine({'t':'line' ,'p1':870005 ,'p2':860005})
    appendLine({'t':'line' ,'p1':860005 ,'p2':850005})
    appendLine({'t':'line' ,'p1':850005 ,'p2':840004})
    appendLine({'t':'line' ,'p1':900000 ,'p2':920000})
    appendLine({'t':'line' ,'p1':920000 ,'p2':930000})
    appendLine({'t':'line' ,'p1':930000 ,'p2':940000})
    appendLine({'t':'line' ,'p1':900000 ,'p2':900002})
    appendLine({'t':'line' ,'p1':900002 ,'p2':900003})
    appendLine({'t':'line' ,'p1':900003 ,'p2':900004})
    appendLine({'t':'line' ,'p1':900004 ,'p2':940004})
    appendLine({'t':'line' ,'p1':940004 ,'p2':940000})
    appendLine({'t':'line' ,'p1':940000 ,'p2':950001})
    appendLine({'t':'line' ,'p1':950001 ,'p2':950002})
    appendLine({'t':'line' ,'p1':950002 ,'p2':950003})
    appendLine({'t':'line' ,'p1':950003 ,'p2':950005})
    appendLine({'t':'line' ,'p1':950005 ,'p2':940004})
    appendLine({'t':'line' ,'p1':950005 ,'p2':930005})
    appendLine({'t':'line' ,'p1':930005 ,'p2':920005})
    appendLine({'t':'line' ,'p1':920005 ,'p2':910005})
    appendLine({'t':'line' ,'p1':910005 ,'p2':900004})
    appendLine({'t':'line' ,'p1':840006 ,'p2':860006})
    appendLine({'t':'line' ,'p1':860006 ,'p2':870006})
    appendLine({'t':'line' ,'p1':870006 ,'p2':880006})
    appendLine({'t':'line' ,'p1':840006 ,'p2':840008})
    appendLine({'t':'line' ,'p1':840008 ,'p2':840009})
    appendLine({'t':'line' ,'p1':840009 ,'p2':840010})
    appendLine({'t':'line' ,'p1':840010 ,'p2':880010})
    appendLine({'t':'line' ,'p1':880010 ,'p2':880006})
    appendLine({'t':'line' ,'p1':880006 ,'p2':890007})
    appendLine({'t':'line' ,'p1':890007 ,'p2':890008})
    appendLine({'t':'line' ,'p1':890008 ,'p2':890009})
    appendLine({'t':'line' ,'p1':890009 ,'p2':890011})
    appendLine({'t':'line' ,'p1':890011 ,'p2':880010})
    appendLine({'t':'line' ,'p1':890011 ,'p2':870011})
    appendLine({'t':'line' ,'p1':870011 ,'p2':860011})
    appendLine({'t':'line' ,'p1':860011 ,'p2':850011})
    appendLine({'t':'line' ,'p1':850011 ,'p2':840010})
    appendLine({'t':'line' ,'p1':900006 ,'p2':920006})
    appendLine({'t':'line' ,'p1':920006 ,'p2':930006})
    appendLine({'t':'line' ,'p1':930006 ,'p2':940006})
    appendLine({'t':'line' ,'p1':900006 ,'p2':900008})
    appendLine({'t':'line' ,'p1':900008 ,'p2':900009})
    appendLine({'t':'line' ,'p1':900009 ,'p2':900010})
    appendLine({'t':'line' ,'p1':900010 ,'p2':940010})
    appendLine({'t':'line' ,'p1':940010 ,'p2':940006})
    appendLine({'t':'line' ,'p1':940006 ,'p2':950007})
    appendLine({'t':'line' ,'p1':950007 ,'p2':950008})
    appendLine({'t':'line' ,'p1':950008 ,'p2':950009})
    appendLine({'t':'line' ,'p1':950009 ,'p2':950011})
    appendLine({'t':'line' ,'p1':950011 ,'p2':940010})
    appendLine({'t':'line' ,'p1':950011 ,'p2':930011})
    appendLine({'t':'line' ,'p1':930011 ,'p2':920011})
    appendLine({'t':'line' ,'p1':920011 ,'p2':910011})
    appendLine({'t':'line' ,'p1':910011 ,'p2':900010})
    appendLine({'t':'line' ,'p1':830003 ,'p2':840003})
    appendLine({'t':'line' ,'p1':810005 ,'p2':810006})
    appendLine({'t':'line' ,'p1':860005 ,'p2':860006})
    appendLine({'t':'line' ,'p1':830008 ,'p2':840008})
    appendLine({'t':'line' ,'p1':1010003 ,'p2':1020003})
    appendLine({'t':'line' ,'p1':990005 ,'p2':990006})
    appendLine({'t':'line' ,'p1':1040005 ,'p2':1040006})
    appendLine({'t':'line' ,'p1':1010008 ,'p2':1020008})
    appendLine({'t':'line' ,'p1':960000 ,'p2':980000})
    appendLine({'t':'line' ,'p1':980000 ,'p2':990000})
    appendLine({'t':'line' ,'p1':990000 ,'p2':1000000})
    appendLine({'t':'line' ,'p1':960000 ,'p2':960002})
    appendLine({'t':'line' ,'p1':960002 ,'p2':960003})
    appendLine({'t':'line' ,'p1':960003 ,'p2':960004})
    appendLine({'t':'line' ,'p1':960004 ,'p2':1000004})
    appendLine({'t':'line' ,'p1':1000004 ,'p2':1000000})
    appendLine({'t':'line' ,'p1':1000000 ,'p2':1010001})
    appendLine({'t':'line' ,'p1':1010001 ,'p2':1010002})
    appendLine({'t':'line' ,'p1':1010002 ,'p2':1010003})
    appendLine({'t':'line' ,'p1':1010003 ,'p2':1010005})
    appendLine({'t':'line' ,'p1':1010005 ,'p2':1000004})
    appendLine({'t':'line' ,'p1':1010005 ,'p2':990005})
    appendLine({'t':'line' ,'p1':990005 ,'p2':980005})
    appendLine({'t':'line' ,'p1':980005 ,'p2':970005})
    appendLine({'t':'line' ,'p1':970005 ,'p2':960004})
    appendLine({'t':'line' ,'p1':1020000 ,'p2':1040000})
    appendLine({'t':'line' ,'p1':1040000 ,'p2':1050000})
    appendLine({'t':'line' ,'p1':1050000 ,'p2':1060000})
    appendLine({'t':'line' ,'p1':1020000 ,'p2':1020002})
    appendLine({'t':'line' ,'p1':1020002 ,'p2':1020003})
    appendLine({'t':'line' ,'p1':1020003 ,'p2':1020004})
    appendLine({'t':'line' ,'p1':1020004 ,'p2':1060004})
    appendLine({'t':'line' ,'p1':1060004 ,'p2':1060000})
    appendLine({'t':'line' ,'p1':1060000 ,'p2':1070001})
    appendLine({'t':'line' ,'p1':1070001 ,'p2':1070002})
    appendLine({'t':'line' ,'p1':1070002 ,'p2':1070003})
    appendLine({'t':'line' ,'p1':1070003 ,'p2':1070005})
    appendLine({'t':'line' ,'p1':1070005 ,'p2':1060004})
    appendLine({'t':'line' ,'p1':1070005 ,'p2':1050005})
    appendLine({'t':'line' ,'p1':1050005 ,'p2':1040005})
    appendLine({'t':'line' ,'p1':1040005 ,'p2':1030005})
    appendLine({'t':'line' ,'p1':1030005 ,'p2':1020004})
    appendLine({'t':'line' ,'p1':960006 ,'p2':980006})
    appendLine({'t':'line' ,'p1':980006 ,'p2':990006})
    appendLine({'t':'line' ,'p1':990006 ,'p2':1000006})
    appendLine({'t':'line' ,'p1':960006 ,'p2':960008})
    appendLine({'t':'line' ,'p1':960008 ,'p2':960009})
    appendLine({'t':'line' ,'p1':960009 ,'p2':960010})
    appendLine({'t':'line' ,'p1':960010 ,'p2':1000010})
    appendLine({'t':'line' ,'p1':1000010 ,'p2':1000006})
    appendLine({'t':'line' ,'p1':1000006 ,'p2':1010007})
    appendLine({'t':'line' ,'p1':1010007 ,'p2':1010008})
    appendLine({'t':'line' ,'p1':1010008 ,'p2':1010009})
    appendLine({'t':'line' ,'p1':1010009 ,'p2':1010011})
    appendLine({'t':'line' ,'p1':1010011 ,'p2':1000010})
    appendLine({'t':'line' ,'p1':1010011 ,'p2':990011})
    appendLine({'t':'line' ,'p1':990011 ,'p2':980011})
    appendLine({'t':'line' ,'p1':980011 ,'p2':970011})
    appendLine({'t':'line' ,'p1':970011 ,'p2':960010})
    appendLine({'t':'line' ,'p1':1020006 ,'p2':1040006})
    appendLine({'t':'line' ,'p1':1040006 ,'p2':1050006})
    appendLine({'t':'line' ,'p1':1050006 ,'p2':1060006})
    appendLine({'t':'line' ,'p1':1020006 ,'p2':1020008})
    appendLine({'t':'line' ,'p1':1020008 ,'p2':1020009})
    appendLine({'t':'line' ,'p1':1020009 ,'p2':1020010})
    appendLine({'t':'line' ,'p1':1020010 ,'p2':1060010})
    appendLine({'t':'line' ,'p1':1060010 ,'p2':1060006})
    appendLine({'t':'line' ,'p1':1060006 ,'p2':1070007})
    appendLine({'t':'line' ,'p1':1070007 ,'p2':1070008})
    appendLine({'t':'line' ,'p1':1070008 ,'p2':1070009})
    appendLine({'t':'line' ,'p1':1070009 ,'p2':1070011})
    appendLine({'t':'line' ,'p1':1070011 ,'p2':1060010})
    appendLine({'t':'line' ,'p1':1070011 ,'p2':1050011})
    appendLine({'t':'line' ,'p1':1050011 ,'p2':1040011})
    appendLine({'t':'line' ,'p1':1040011 ,'p2':1030011})
    appendLine({'t':'line' ,'p1':1030011 ,'p2':1020010})
    appendLine({'t':'line' ,'p1':950003 ,'p2':960003})
    appendLine({'t':'line' ,'p1':930005 ,'p2':930006})
    appendLine({'t':'line' ,'p1':980005 ,'p2':980006})
    appendLine({'t':'line' ,'p1':950008 ,'p2':960008})
    appendLine({'t':'line' ,'p1':1130003 ,'p2':1140003})
    appendLine({'t':'line' ,'p1':1110005 ,'p2':1110006})
    appendLine({'t':'line' ,'p1':1160005 ,'p2':1160006})
    appendLine({'t':'line' ,'p1':1130008 ,'p2':1140008})
    appendLine({'t':'line' ,'p1':1080000 ,'p2':1100000})
    appendLine({'t':'line' ,'p1':1100000 ,'p2':1110000})
    appendLine({'t':'line' ,'p1':1110000 ,'p2':1120000})
    appendLine({'t':'line' ,'p1':1080000 ,'p2':1080002})
    appendLine({'t':'line' ,'p1':1080002 ,'p2':1080003})
    appendLine({'t':'line' ,'p1':1080003 ,'p2':1080004})
    appendLine({'t':'line' ,'p1':1080004 ,'p2':1120004})
    appendLine({'t':'line' ,'p1':1120004 ,'p2':1120000})
    appendLine({'t':'line' ,'p1':1120000 ,'p2':1130001})
    appendLine({'t':'line' ,'p1':1130001 ,'p2':1130002})
    appendLine({'t':'line' ,'p1':1130002 ,'p2':1130003})
    appendLine({'t':'line' ,'p1':1130003 ,'p2':1130005})
    appendLine({'t':'line' ,'p1':1130005 ,'p2':1120004})
    appendLine({'t':'line' ,'p1':1130005 ,'p2':1110005})
    appendLine({'t':'line' ,'p1':1110005 ,'p2':1100005})
    appendLine({'t':'line' ,'p1':1100005 ,'p2':1090005})
    appendLine({'t':'line' ,'p1':1090005 ,'p2':1080004})
    appendLine({'t':'line' ,'p1':1140000 ,'p2':1160000})
    appendLine({'t':'line' ,'p1':1160000 ,'p2':1170000})
    appendLine({'t':'line' ,'p1':1170000 ,'p2':1180000})
    appendLine({'t':'line' ,'p1':1140000 ,'p2':1140002})
    appendLine({'t':'line' ,'p1':1140002 ,'p2':1140003})
    appendLine({'t':'line' ,'p1':1140003 ,'p2':1140004})
    appendLine({'t':'line' ,'p1':1140004 ,'p2':1180004})
    appendLine({'t':'line' ,'p1':1180004 ,'p2':1180000})
    appendLine({'t':'line' ,'p1':1180000 ,'p2':1190001})
    appendLine({'t':'line' ,'p1':1190001 ,'p2':1190002})
    appendLine({'t':'line' ,'p1':1190002 ,'p2':1190003})
    appendLine({'t':'line' ,'p1':1190003 ,'p2':1190005})
    appendLine({'t':'line' ,'p1':1190005 ,'p2':1180004})
    appendLine({'t':'line' ,'p1':1190005 ,'p2':1170005})
    appendLine({'t':'line' ,'p1':1170005 ,'p2':1160005})
    appendLine({'t':'line' ,'p1':1160005 ,'p2':1150005})
    appendLine({'t':'line' ,'p1':1150005 ,'p2':1140004})
    appendLine({'t':'line' ,'p1':1080006 ,'p2':1100006})
    appendLine({'t':'line' ,'p1':1100006 ,'p2':1110006})
    appendLine({'t':'line' ,'p1':1110006 ,'p2':1120006})
    appendLine({'t':'line' ,'p1':1080006 ,'p2':1080008})
    appendLine({'t':'line' ,'p1':1080008 ,'p2':1080009})
    appendLine({'t':'line' ,'p1':1080009 ,'p2':1080010})
    appendLine({'t':'line' ,'p1':1080010 ,'p2':1120010})
    appendLine({'t':'line' ,'p1':1120010 ,'p2':1120006})
    appendLine({'t':'line' ,'p1':1120006 ,'p2':1130007})
    appendLine({'t':'line' ,'p1':1130007 ,'p2':1130008})
    appendLine({'t':'line' ,'p1':1130008 ,'p2':1130009})
    appendLine({'t':'line' ,'p1':1130009 ,'p2':1130011})
    appendLine({'t':'line' ,'p1':1130011 ,'p2':1120010})
    appendLine({'t':'line' ,'p1':1130011 ,'p2':1110011})
    appendLine({'t':'line' ,'p1':1110011 ,'p2':1100011})
    appendLine({'t':'line' ,'p1':1100011 ,'p2':1090011})
    appendLine({'t':'line' ,'p1':1090011 ,'p2':1080010})
    appendLine({'t':'line' ,'p1':1140006 ,'p2':1160006})
    appendLine({'t':'line' ,'p1':1160006 ,'p2':1170006})
    appendLine({'t':'line' ,'p1':1170006 ,'p2':1180006})
    appendLine({'t':'line' ,'p1':1140006 ,'p2':1140008})
    appendLine({'t':'line' ,'p1':1140008 ,'p2':1140009})
    appendLine({'t':'line' ,'p1':1140009 ,'p2':1140010})
    appendLine({'t':'line' ,'p1':1140010 ,'p2':1180010})
    appendLine({'t':'line' ,'p1':1180010 ,'p2':1180006})
    appendLine({'t':'line' ,'p1':1180006 ,'p2':1190007})
    appendLine({'t':'line' ,'p1':1190007 ,'p2':1190008})
    appendLine({'t':'line' ,'p1':1190008 ,'p2':1190009})
    appendLine({'t':'line' ,'p1':1190009 ,'p2':1190011})
    appendLine({'t':'line' ,'p1':1190011 ,'p2':1180010})
    appendLine({'t':'line' ,'p1':1190011 ,'p2':1170011})
    appendLine({'t':'line' ,'p1':1170011 ,'p2':1160011})
    appendLine({'t':'line' ,'p1':1160011 ,'p2':1150011})
    appendLine({'t':'line' ,'p1':1150011 ,'p2':1140010})
    appendLine({'t':'line' ,'p1':1070003 ,'p2':1080003})
    appendLine({'t':'line' ,'p1':1050005 ,'p2':1050006})
    appendLine({'t':'line' ,'p1':1100005 ,'p2':1100006})
    appendLine({'t':'line' ,'p1':1070008 ,'p2':1080008})
    appendLine({'t':'line' ,'p1':1250003 ,'p2':1260003})
    appendLine({'t':'line' ,'p1':1230005 ,'p2':1230006})
    appendLine({'t':'line' ,'p1':1280005 ,'p2':1280006})
    appendLine({'t':'line' ,'p1':1250008 ,'p2':1260008})
    appendLine({'t':'line' ,'p1':1200000 ,'p2':1220000})
    appendLine({'t':'line' ,'p1':1220000 ,'p2':1230000})
    appendLine({'t':'line' ,'p1':1230000 ,'p2':1240000})
    appendLine({'t':'line' ,'p1':1200000 ,'p2':1200002})
    appendLine({'t':'line' ,'p1':1200002 ,'p2':1200003})
    appendLine({'t':'line' ,'p1':1200003 ,'p2':1200004})
    appendLine({'t':'line' ,'p1':1200004 ,'p2':1240004})
    appendLine({'t':'line' ,'p1':1240004 ,'p2':1240000})
    appendLine({'t':'line' ,'p1':1240000 ,'p2':1250001})
    appendLine({'t':'line' ,'p1':1250001 ,'p2':1250002})
    appendLine({'t':'line' ,'p1':1250002 ,'p2':1250003})
    appendLine({'t':'line' ,'p1':1250003 ,'p2':1250005})
    appendLine({'t':'line' ,'p1':1250005 ,'p2':1240004})
    appendLine({'t':'line' ,'p1':1250005 ,'p2':1230005})
    appendLine({'t':'line' ,'p1':1230005 ,'p2':1220005})
    appendLine({'t':'line' ,'p1':1220005 ,'p2':1210005})
    appendLine({'t':'line' ,'p1':1210005 ,'p2':1200004})
    appendLine({'t':'line' ,'p1':1260000 ,'p2':1280000})
    appendLine({'t':'line' ,'p1':1280000 ,'p2':1290000})
    appendLine({'t':'line' ,'p1':1290000 ,'p2':1300000})
    appendLine({'t':'line' ,'p1':1260000 ,'p2':1260002})
    appendLine({'t':'line' ,'p1':1260002 ,'p2':1260003})
    appendLine({'t':'line' ,'p1':1260003 ,'p2':1260004})
    appendLine({'t':'line' ,'p1':1260004 ,'p2':1300004})
    appendLine({'t':'line' ,'p1':1300004 ,'p2':1300000})
    appendLine({'t':'line' ,'p1':1300000 ,'p2':1310001})
    appendLine({'t':'line' ,'p1':1310001 ,'p2':1310002})
    appendLine({'t':'line' ,'p1':1310002 ,'p2':1310003})
    appendLine({'t':'line' ,'p1':1310003 ,'p2':1310005})
    appendLine({'t':'line' ,'p1':1310005 ,'p2':1300004})
    appendLine({'t':'line' ,'p1':1310005 ,'p2':1290005})
    appendLine({'t':'line' ,'p1':1290005 ,'p2':1280005})
    appendLine({'t':'line' ,'p1':1280005 ,'p2':1270005})
    appendLine({'t':'line' ,'p1':1270005 ,'p2':1260004})
    appendLine({'t':'line' ,'p1':1200006 ,'p2':1220006})
    appendLine({'t':'line' ,'p1':1220006 ,'p2':1230006})
    appendLine({'t':'line' ,'p1':1230006 ,'p2':1240006})
    appendLine({'t':'line' ,'p1':1200006 ,'p2':1200008})
    appendLine({'t':'line' ,'p1':1200008 ,'p2':1200009})
    appendLine({'t':'line' ,'p1':1200009 ,'p2':1200010})
    appendLine({'t':'line' ,'p1':1200010 ,'p2':1240010})
    appendLine({'t':'line' ,'p1':1240010 ,'p2':1240006})
    appendLine({'t':'line' ,'p1':1240006 ,'p2':1250007})
    appendLine({'t':'line' ,'p1':1250007 ,'p2':1250008})
    appendLine({'t':'line' ,'p1':1250008 ,'p2':1250009})
    appendLine({'t':'line' ,'p1':1250009 ,'p2':1250011})
    appendLine({'t':'line' ,'p1':1250011 ,'p2':1240010})
    appendLine({'t':'line' ,'p1':1250011 ,'p2':1230011})
    appendLine({'t':'line' ,'p1':1230011 ,'p2':1220011})
    appendLine({'t':'line' ,'p1':1220011 ,'p2':1210011})
    appendLine({'t':'line' ,'p1':1210011 ,'p2':1200010})
    appendLine({'t':'line' ,'p1':1260006 ,'p2':1280006})
    appendLine({'t':'line' ,'p1':1280006 ,'p2':1290006})
    appendLine({'t':'line' ,'p1':1290006 ,'p2':1300006})
    appendLine({'t':'line' ,'p1':1260006 ,'p2':1260008})
    appendLine({'t':'line' ,'p1':1260008 ,'p2':1260009})
    appendLine({'t':'line' ,'p1':1260009 ,'p2':1260010})
    appendLine({'t':'line' ,'p1':1260010 ,'p2':1300010})
    appendLine({'t':'line' ,'p1':1300010 ,'p2':1300006})
    appendLine({'t':'line' ,'p1':1300006 ,'p2':1310007})
    appendLine({'t':'line' ,'p1':1310007 ,'p2':1310008})
    appendLine({'t':'line' ,'p1':1310008 ,'p2':1310009})
    appendLine({'t':'line' ,'p1':1310009 ,'p2':1310011})
    appendLine({'t':'line' ,'p1':1310011 ,'p2':1300010})
    appendLine({'t':'line' ,'p1':1310011 ,'p2':1290011})
    appendLine({'t':'line' ,'p1':1290011 ,'p2':1280011})
    appendLine({'t':'line' ,'p1':1280011 ,'p2':1270011})
    appendLine({'t':'line' ,'p1':1270011 ,'p2':1260010})
    appendLine({'t':'line' ,'p1':1190003 ,'p2':1200003})
    appendLine({'t':'line' ,'p1':1170005 ,'p2':1170006})
    appendLine({'t':'line' ,'p1':1220005 ,'p2':1220006})
    appendLine({'t':'line' ,'p1':1190008 ,'p2':1200008})
    appendLine({'t':'line' ,'p1':1370003 ,'p2':1380003})
    appendLine({'t':'line' ,'p1':1350005 ,'p2':1350006})
    appendLine({'t':'line' ,'p1':1400005 ,'p2':1400006})
    appendLine({'t':'line' ,'p1':1370008 ,'p2':1380008})
    appendLine({'t':'line' ,'p1':1320000 ,'p2':1340000})
    appendLine({'t':'line' ,'p1':1340000 ,'p2':1350000})
    appendLine({'t':'line' ,'p1':1350000 ,'p2':1360000})
    appendLine({'t':'line' ,'p1':1320000 ,'p2':1320002})
    appendLine({'t':'line' ,'p1':1320002 ,'p2':1320003})
    appendLine({'t':'line' ,'p1':1320003 ,'p2':1320004})
    appendLine({'t':'line' ,'p1':1320004 ,'p2':1360004})
    appendLine({'t':'line' ,'p1':1360004 ,'p2':1360000})
    appendLine({'t':'line' ,'p1':1360000 ,'p2':1370001})
    appendLine({'t':'line' ,'p1':1370001 ,'p2':1370002})
    appendLine({'t':'line' ,'p1':1370002 ,'p2':1370003})
    appendLine({'t':'line' ,'p1':1370003 ,'p2':1370005})
    appendLine({'t':'line' ,'p1':1370005 ,'p2':1360004})
    appendLine({'t':'line' ,'p1':1370005 ,'p2':1350005})
    appendLine({'t':'line' ,'p1':1350005 ,'p2':1340005})
    appendLine({'t':'line' ,'p1':1340005 ,'p2':1330005})
    appendLine({'t':'line' ,'p1':1330005 ,'p2':1320004})
    appendLine({'t':'line' ,'p1':1380000 ,'p2':1400000})
    appendLine({'t':'line' ,'p1':1400000 ,'p2':1410000})
    appendLine({'t':'line' ,'p1':1410000 ,'p2':1420000})
    appendLine({'t':'line' ,'p1':1380000 ,'p2':1380002})
    appendLine({'t':'line' ,'p1':1380002 ,'p2':1380003})
    appendLine({'t':'line' ,'p1':1380003 ,'p2':1380004})
    appendLine({'t':'line' ,'p1':1380004 ,'p2':1420004})
    appendLine({'t':'line' ,'p1':1420004 ,'p2':1420000})
    appendLine({'t':'line' ,'p1':1420000 ,'p2':1430001})
    appendLine({'t':'line' ,'p1':1430001 ,'p2':1430002})
    appendLine({'t':'line' ,'p1':1430002 ,'p2':1430003})
    appendLine({'t':'line' ,'p1':1430003 ,'p2':1430005})
    appendLine({'t':'line' ,'p1':1430005 ,'p2':1420004})
    appendLine({'t':'line' ,'p1':1430005 ,'p2':1410005})
    appendLine({'t':'line' ,'p1':1410005 ,'p2':1400005})
    appendLine({'t':'line' ,'p1':1400005 ,'p2':1390005})
    appendLine({'t':'line' ,'p1':1390005 ,'p2':1380004})
    appendLine({'t':'line' ,'p1':1320006 ,'p2':1340006})
    appendLine({'t':'line' ,'p1':1340006 ,'p2':1350006})
    appendLine({'t':'line' ,'p1':1350006 ,'p2':1360006})
    appendLine({'t':'line' ,'p1':1320006 ,'p2':1320008})
    appendLine({'t':'line' ,'p1':1320008 ,'p2':1320009})
    appendLine({'t':'line' ,'p1':1320009 ,'p2':1320010})
    appendLine({'t':'line' ,'p1':1320010 ,'p2':1360010})
    appendLine({'t':'line' ,'p1':1360010 ,'p2':1360006})
    appendLine({'t':'line' ,'p1':1360006 ,'p2':1370007})
    appendLine({'t':'line' ,'p1':1370007 ,'p2':1370008})
    appendLine({'t':'line' ,'p1':1370008 ,'p2':1370009})
    appendLine({'t':'line' ,'p1':1370009 ,'p2':1370011})
    appendLine({'t':'line' ,'p1':1370011 ,'p2':1360010})
    appendLine({'t':'line' ,'p1':1370011 ,'p2':1350011})
    appendLine({'t':'line' ,'p1':1350011 ,'p2':1340011})
    appendLine({'t':'line' ,'p1':1340011 ,'p2':1330011})
    appendLine({'t':'line' ,'p1':1330011 ,'p2':1320010})
    appendLine({'t':'line' ,'p1':1380006 ,'p2':1400006})
    appendLine({'t':'line' ,'p1':1400006 ,'p2':1410006})
    appendLine({'t':'line' ,'p1':1410006 ,'p2':1420006})
    appendLine({'t':'line' ,'p1':1380006 ,'p2':1380008})
    appendLine({'t':'line' ,'p1':1380008 ,'p2':1380009})
    appendLine({'t':'line' ,'p1':1380009 ,'p2':1380010})
    appendLine({'t':'line' ,'p1':1380010 ,'p2':1420010})
    appendLine({'t':'line' ,'p1':1420010 ,'p2':1420006})
    appendLine({'t':'line' ,'p1':1420006 ,'p2':1430007})
    appendLine({'t':'line' ,'p1':1430007 ,'p2':1430008})
    appendLine({'t':'line' ,'p1':1430008 ,'p2':1430009})
    appendLine({'t':'line' ,'p1':1430009 ,'p2':1430011})
    appendLine({'t':'line' ,'p1':1430011 ,'p2':1420010})
    appendLine({'t':'line' ,'p1':1430011 ,'p2':1410011})
    appendLine({'t':'line' ,'p1':1410011 ,'p2':1400011})
    appendLine({'t':'line' ,'p1':1400011 ,'p2':1390011})
    appendLine({'t':'line' ,'p1':1390011 ,'p2':1380010})
    appendLine({'t':'line' ,'p1':1310003 ,'p2':1320003})
    appendLine({'t':'line' ,'p1':1290005 ,'p2':1290006})
    appendLine({'t':'line' ,'p1':1340005 ,'p2':1340006})
    appendLine({'t':'line' ,'p1':1310008 ,'p2':1320008})
    appendLine({'t':'line' ,'p1':1490003 ,'p2':1500003})
    appendLine({'t':'line' ,'p1':1470005 ,'p2':1470006})
    appendLine({'t':'line' ,'p1':1520005 ,'p2':1520006})
    appendLine({'t':'line' ,'p1':1490008 ,'p2':1500008})
    appendLine({'t':'line' ,'p1':1440000 ,'p2':1460000})
    appendLine({'t':'line' ,'p1':1460000 ,'p2':1470000})
    appendLine({'t':'line' ,'p1':1470000 ,'p2':1480000})
    appendLine({'t':'line' ,'p1':1440000 ,'p2':1440002})
    appendLine({'t':'line' ,'p1':1440002 ,'p2':1440003})
    appendLine({'t':'line' ,'p1':1440003 ,'p2':1440004})
    appendLine({'t':'line' ,'p1':1440004 ,'p2':1480004})
    appendLine({'t':'line' ,'p1':1480004 ,'p2':1480000})
    appendLine({'t':'line' ,'p1':1480000 ,'p2':1490001})
    appendLine({'t':'line' ,'p1':1490001 ,'p2':1490002})
    appendLine({'t':'line' ,'p1':1490002 ,'p2':1490003})
    appendLine({'t':'line' ,'p1':1490003 ,'p2':1490005})
    appendLine({'t':'line' ,'p1':1490005 ,'p2':1480004})
    appendLine({'t':'line' ,'p1':1490005 ,'p2':1470005})
    appendLine({'t':'line' ,'p1':1470005 ,'p2':1460005})
    appendLine({'t':'line' ,'p1':1460005 ,'p2':1450005})
    appendLine({'t':'line' ,'p1':1450005 ,'p2':1440004})
    appendLine({'t':'line' ,'p1':1500000 ,'p2':1520000})
    appendLine({'t':'line' ,'p1':1520000 ,'p2':1530000})
    appendLine({'t':'line' ,'p1':1530000 ,'p2':1540000})
    appendLine({'t':'line' ,'p1':1500000 ,'p2':1500002})
    appendLine({'t':'line' ,'p1':1500002 ,'p2':1500003})
    appendLine({'t':'line' ,'p1':1500003 ,'p2':1500004})
    appendLine({'t':'line' ,'p1':1500004 ,'p2':1540004})
    appendLine({'t':'line' ,'p1':1540004 ,'p2':1540000})
    appendLine({'t':'line' ,'p1':1540000 ,'p2':1550001})
    appendLine({'t':'line' ,'p1':1550001 ,'p2':1550002})
    appendLine({'t':'line' ,'p1':1550002 ,'p2':1550003})
    appendLine({'t':'line' ,'p1':1550003 ,'p2':1550005})
    appendLine({'t':'line' ,'p1':1550005 ,'p2':1540004})
    appendLine({'t':'line' ,'p1':1550005 ,'p2':1530005})
    appendLine({'t':'line' ,'p1':1530005 ,'p2':1520005})
    appendLine({'t':'line' ,'p1':1520005 ,'p2':1510005})
    appendLine({'t':'line' ,'p1':1510005 ,'p2':1500004})
    appendLine({'t':'line' ,'p1':1440006 ,'p2':1460006})
    appendLine({'t':'line' ,'p1':1460006 ,'p2':1470006})
    appendLine({'t':'line' ,'p1':1470006 ,'p2':1480006})
    appendLine({'t':'line' ,'p1':1440006 ,'p2':1440008})
    appendLine({'t':'line' ,'p1':1440008 ,'p2':1440009})
    appendLine({'t':'line' ,'p1':1440009 ,'p2':1440010})
    appendLine({'t':'line' ,'p1':1440010 ,'p2':1480010})
    appendLine({'t':'line' ,'p1':1480010 ,'p2':1480006})
    appendLine({'t':'line' ,'p1':1480006 ,'p2':1490007})
    appendLine({'t':'line' ,'p1':1490007 ,'p2':1490008})
    appendLine({'t':'line' ,'p1':1490008 ,'p2':1490009})
    appendLine({'t':'line' ,'p1':1490009 ,'p2':1490011})
    appendLine({'t':'line' ,'p1':1490011 ,'p2':1480010})
    appendLine({'t':'line' ,'p1':1490011 ,'p2':1470011})
    appendLine({'t':'line' ,'p1':1470011 ,'p2':1460011})
    appendLine({'t':'line' ,'p1':1460011 ,'p2':1450011})
    appendLine({'t':'line' ,'p1':1450011 ,'p2':1440010})
    appendLine({'t':'line' ,'p1':1500006 ,'p2':1520006})
    appendLine({'t':'line' ,'p1':1520006 ,'p2':1530006})
    appendLine({'t':'line' ,'p1':1530006 ,'p2':1540006})
    appendLine({'t':'line' ,'p1':1500006 ,'p2':1500008})
    appendLine({'t':'line' ,'p1':1500008 ,'p2':1500009})
    appendLine({'t':'line' ,'p1':1500009 ,'p2':1500010})
    appendLine({'t':'line' ,'p1':1500010 ,'p2':1540010})
    appendLine({'t':'line' ,'p1':1540010 ,'p2':1540006})
    appendLine({'t':'line' ,'p1':1540006 ,'p2':1550007})
    appendLine({'t':'line' ,'p1':1550007 ,'p2':1550008})
    appendLine({'t':'line' ,'p1':1550008 ,'p2':1550009})
    appendLine({'t':'line' ,'p1':1550009 ,'p2':1550011})
    appendLine({'t':'line' ,'p1':1550011 ,'p2':1540010})
    appendLine({'t':'line' ,'p1':1550011 ,'p2':1530011})
    appendLine({'t':'line' ,'p1':1530011 ,'p2':1520011})
    appendLine({'t':'line' ,'p1':1520011 ,'p2':1510011})
    appendLine({'t':'line' ,'p1':1510011 ,'p2':1500010})
    appendLine({'t':'line' ,'p1':1430003 ,'p2':1440003})
    appendLine({'t':'line' ,'p1':1410005 ,'p2':1410006})
    appendLine({'t':'line' ,'p1':1460005 ,'p2':1460006})
    appendLine({'t':'line' ,'p1':1430008 ,'p2':1440008})
    appendLine({'t':'line' ,'p1':1610003 ,'p2':1620003})
    appendLine({'t':'line' ,'p1':1590005 ,'p2':1590006})
    appendLine({'t':'line' ,'p1':1640005 ,'p2':1640006})
    appendLine({'t':'line' ,'p1':1610008 ,'p2':1620008})
    appendLine({'t':'line' ,'p1':1560000 ,'p2':1580000})
    appendLine({'t':'line' ,'p1':1580000 ,'p2':1590000})
    appendLine({'t':'line' ,'p1':1590000 ,'p2':1600000})
    appendLine({'t':'line' ,'p1':1560000 ,'p2':1560002})
    appendLine({'t':'line' ,'p1':1560002 ,'p2':1560003})
    appendLine({'t':'line' ,'p1':1560003 ,'p2':1560004})
    appendLine({'t':'line' ,'p1':1560004 ,'p2':1600004})
    appendLine({'t':'line' ,'p1':1600004 ,'p2':1600000})
    appendLine({'t':'line' ,'p1':1600000 ,'p2':1610001})
    appendLine({'t':'line' ,'p1':1610001 ,'p2':1610002})
    appendLine({'t':'line' ,'p1':1610002 ,'p2':1610003})
    appendLine({'t':'line' ,'p1':1610003 ,'p2':1610005})
    appendLine({'t':'line' ,'p1':1610005 ,'p2':1600004})
    appendLine({'t':'line' ,'p1':1610005 ,'p2':1590005})
    appendLine({'t':'line' ,'p1':1590005 ,'p2':1580005})
    appendLine({'t':'line' ,'p1':1580005 ,'p2':1570005})
    appendLine({'t':'line' ,'p1':1570005 ,'p2':1560004})
    appendLine({'t':'line' ,'p1':1620000 ,'p2':1640000})
    appendLine({'t':'line' ,'p1':1640000 ,'p2':1650000})
    appendLine({'t':'line' ,'p1':1650000 ,'p2':1660000})
    appendLine({'t':'line' ,'p1':1620000 ,'p2':1620002})
    appendLine({'t':'line' ,'p1':1620002 ,'p2':1620003})
    appendLine({'t':'line' ,'p1':1620003 ,'p2':1620004})
    appendLine({'t':'line' ,'p1':1620004 ,'p2':1660004})
    appendLine({'t':'line' ,'p1':1660004 ,'p2':1660000})
    appendLine({'t':'line' ,'p1':1660000 ,'p2':1670001})
    appendLine({'t':'line' ,'p1':1670001 ,'p2':1670002})
    appendLine({'t':'line' ,'p1':1670002 ,'p2':1670003})
    appendLine({'t':'line' ,'p1':1670003 ,'p2':1670005})
    appendLine({'t':'line' ,'p1':1670005 ,'p2':1660004})
    appendLine({'t':'line' ,'p1':1670005 ,'p2':1650005})
    appendLine({'t':'line' ,'p1':1650005 ,'p2':1640005})
    appendLine({'t':'line' ,'p1':1640005 ,'p2':1630005})
    appendLine({'t':'line' ,'p1':1630005 ,'p2':1620004})
    appendLine({'t':'line' ,'p1':1560006 ,'p2':1580006})
    appendLine({'t':'line' ,'p1':1580006 ,'p2':1590006})
    appendLine({'t':'line' ,'p1':1590006 ,'p2':1600006})
    appendLine({'t':'line' ,'p1':1560006 ,'p2':1560008})
    appendLine({'t':'line' ,'p1':1560008 ,'p2':1560009})
    appendLine({'t':'line' ,'p1':1560009 ,'p2':1560010})
    appendLine({'t':'line' ,'p1':1560010 ,'p2':1600010})
    appendLine({'t':'line' ,'p1':1600010 ,'p2':1600006})
    appendLine({'t':'line' ,'p1':1600006 ,'p2':1610007})
    appendLine({'t':'line' ,'p1':1610007 ,'p2':1610008})
    appendLine({'t':'line' ,'p1':1610008 ,'p2':1610009})
    appendLine({'t':'line' ,'p1':1610009 ,'p2':1610011})
    appendLine({'t':'line' ,'p1':1610011 ,'p2':1600010})
    appendLine({'t':'line' ,'p1':1610011 ,'p2':1590011})
    appendLine({'t':'line' ,'p1':1590011 ,'p2':1580011})
    appendLine({'t':'line' ,'p1':1580011 ,'p2':1570011})
    appendLine({'t':'line' ,'p1':1570011 ,'p2':1560010})
    appendLine({'t':'line' ,'p1':1620006 ,'p2':1640006})
    appendLine({'t':'line' ,'p1':1640006 ,'p2':1650006})
    appendLine({'t':'line' ,'p1':1650006 ,'p2':1660006})
    appendLine({'t':'line' ,'p1':1620006 ,'p2':1620008})
    appendLine({'t':'line' ,'p1':1620008 ,'p2':1620009})
    appendLine({'t':'line' ,'p1':1620009 ,'p2':1620010})
    appendLine({'t':'line' ,'p1':1620010 ,'p2':1660010})
    appendLine({'t':'line' ,'p1':1660010 ,'p2':1660006})
    appendLine({'t':'line' ,'p1':1660006 ,'p2':1670007})
    appendLine({'t':'line' ,'p1':1670007 ,'p2':1670008})
    appendLine({'t':'line' ,'p1':1670008 ,'p2':1670009})
    appendLine({'t':'line' ,'p1':1670009 ,'p2':1670011})
    appendLine({'t':'line' ,'p1':1670011 ,'p2':1660010})
    appendLine({'t':'line' ,'p1':1670011 ,'p2':1650011})
    appendLine({'t':'line' ,'p1':1650011 ,'p2':1640011})
    appendLine({'t':'line' ,'p1':1640011 ,'p2':1630011})
    appendLine({'t':'line' ,'p1':1630011 ,'p2':1620010})
    appendLine({'t':'line' ,'p1':1550003 ,'p2':1560003})
    appendLine({'t':'line' ,'p1':1530005 ,'p2':1530006})
    appendLine({'t':'line' ,'p1':1580005 ,'p2':1580006})
    appendLine({'t':'line' ,'p1':1550008 ,'p2':1560008})
    appendLine({'t':'line' ,'p1':1730003 ,'p2':1740003})
    appendLine({'t':'line' ,'p1':1710005 ,'p2':1710006})
    appendLine({'t':'line' ,'p1':1760005 ,'p2':1760006})
    appendLine({'t':'line' ,'p1':1730008 ,'p2':1740008})
    appendLine({'t':'line' ,'p1':1680000 ,'p2':1700000})
    appendLine({'t':'line' ,'p1':1700000 ,'p2':1710000})
    appendLine({'t':'line' ,'p1':1710000 ,'p2':1720000})
    appendLine({'t':'line' ,'p1':1680000 ,'p2':1680002})
    appendLine({'t':'line' ,'p1':1680002 ,'p2':1680003})
    appendLine({'t':'line' ,'p1':1680003 ,'p2':1680004})
    appendLine({'t':'line' ,'p1':1680004 ,'p2':1720004})
    appendLine({'t':'line' ,'p1':1720004 ,'p2':1720000})
    appendLine({'t':'line' ,'p1':1720000 ,'p2':1730001})
    appendLine({'t':'line' ,'p1':1730001 ,'p2':1730002})
    appendLine({'t':'line' ,'p1':1730002 ,'p2':1730003})
    appendLine({'t':'line' ,'p1':1730003 ,'p2':1730005})
    appendLine({'t':'line' ,'p1':1730005 ,'p2':1720004})
    appendLine({'t':'line' ,'p1':1730005 ,'p2':1710005})
    appendLine({'t':'line' ,'p1':1710005 ,'p2':1700005})
    appendLine({'t':'line' ,'p1':1700005 ,'p2':1690005})
    appendLine({'t':'line' ,'p1':1690005 ,'p2':1680004})
    appendLine({'t':'line' ,'p1':1740000 ,'p2':1760000})
    appendLine({'t':'line' ,'p1':1760000 ,'p2':1770000})
    appendLine({'t':'line' ,'p1':1770000 ,'p2':1780000})
    appendLine({'t':'line' ,'p1':1740000 ,'p2':1740002})
    appendLine({'t':'line' ,'p1':1740002 ,'p2':1740003})
    appendLine({'t':'line' ,'p1':1740003 ,'p2':1740004})
    appendLine({'t':'line' ,'p1':1740004 ,'p2':1780004})
    appendLine({'t':'line' ,'p1':1780004 ,'p2':1780000})
    appendLine({'t':'line' ,'p1':1780000 ,'p2':1790001})
    appendLine({'t':'line' ,'p1':1790001 ,'p2':1790002})
    appendLine({'t':'line' ,'p1':1790002 ,'p2':1790003})
    appendLine({'t':'line' ,'p1':1790003 ,'p2':1790005})
    appendLine({'t':'line' ,'p1':1790005 ,'p2':1780004})
    appendLine({'t':'line' ,'p1':1790005 ,'p2':1770005})
    appendLine({'t':'line' ,'p1':1770005 ,'p2':1760005})
    appendLine({'t':'line' ,'p1':1760005 ,'p2':1750005})
    appendLine({'t':'line' ,'p1':1750005 ,'p2':1740004})
    appendLine({'t':'line' ,'p1':1680006 ,'p2':1700006})
    appendLine({'t':'line' ,'p1':1700006 ,'p2':1710006})
    appendLine({'t':'line' ,'p1':1710006 ,'p2':1720006})
    appendLine({'t':'line' ,'p1':1680006 ,'p2':1680008})
    appendLine({'t':'line' ,'p1':1680008 ,'p2':1680009})
    appendLine({'t':'line' ,'p1':1680009 ,'p2':1680010})
    appendLine({'t':'line' ,'p1':1680010 ,'p2':1720010})
    appendLine({'t':'line' ,'p1':1720010 ,'p2':1720006})
    appendLine({'t':'line' ,'p1':1720006 ,'p2':1730007})
    appendLine({'t':'line' ,'p1':1730007 ,'p2':1730008})
    appendLine({'t':'line' ,'p1':1730008 ,'p2':1730009})
    appendLine({'t':'line' ,'p1':1730009 ,'p2':1730011})
    appendLine({'t':'line' ,'p1':1730011 ,'p2':1720010})
    appendLine({'t':'line' ,'p1':1730011 ,'p2':1710011})
    appendLine({'t':'line' ,'p1':1710011 ,'p2':1700011})
    appendLine({'t':'line' ,'p1':1700011 ,'p2':1690011})
    appendLine({'t':'line' ,'p1':1690011 ,'p2':1680010})
    appendLine({'t':'line' ,'p1':1740006 ,'p2':1760006})
    appendLine({'t':'line' ,'p1':1760006 ,'p2':1770006})
    appendLine({'t':'line' ,'p1':1770006 ,'p2':1780006})
    appendLine({'t':'line' ,'p1':1740006 ,'p2':1740008})
    appendLine({'t':'line' ,'p1':1740008 ,'p2':1740009})
    appendLine({'t':'line' ,'p1':1740009 ,'p2':1740010})
    appendLine({'t':'line' ,'p1':1740010 ,'p2':1780010})
    appendLine({'t':'line' ,'p1':1780010 ,'p2':1780006})
    appendLine({'t':'line' ,'p1':1780006 ,'p2':1790007})
    appendLine({'t':'line' ,'p1':1790007 ,'p2':1790008})
    appendLine({'t':'line' ,'p1':1790008 ,'p2':1790009})
    appendLine({'t':'line' ,'p1':1790009 ,'p2':1790011})
    appendLine({'t':'line' ,'p1':1790011 ,'p2':1780010})
    appendLine({'t':'line' ,'p1':1790011 ,'p2':1770011})
    appendLine({'t':'line' ,'p1':1770011 ,'p2':1760011})
    appendLine({'t':'line' ,'p1':1760011 ,'p2':1750011})
    appendLine({'t':'line' ,'p1':1750011 ,'p2':1740010})
    appendLine({'t':'line' ,'p1':1670003 ,'p2':1680003})
    appendLine({'t':'line' ,'p1':1650005 ,'p2':1650006})
    appendLine({'t':'line' ,'p1':1700005 ,'p2':1700006})
    appendLine({'t':'line' ,'p1':1670008 ,'p2':1680008})
    appendLine({'t':'line' ,'p1':1850003 ,'p2':1860003})
    appendLine({'t':'line' ,'p1':1830005 ,'p2':1830006})
    appendLine({'t':'line' ,'p1':1880005 ,'p2':1880006})
    appendLine({'t':'line' ,'p1':1850008 ,'p2':1860008})
    appendLine({'t':'line' ,'p1':1800000 ,'p2':1820000})
    appendLine({'t':'line' ,'p1':1820000 ,'p2':1830000})
    appendLine({'t':'line' ,'p1':1830000 ,'p2':1840000})
    appendLine({'t':'line' ,'p1':1800000 ,'p2':1800002})
    appendLine({'t':'line' ,'p1':1800002 ,'p2':1800003})
    appendLine({'t':'line' ,'p1':1800003 ,'p2':1800004})
    appendLine({'t':'line' ,'p1':1800004 ,'p2':1840004})
    appendLine({'t':'line' ,'p1':1840004 ,'p2':1840000})
    appendLine({'t':'line' ,'p1':1840000 ,'p2':1850001})
    appendLine({'t':'line' ,'p1':1850001 ,'p2':1850002})
    appendLine({'t':'line' ,'p1':1850002 ,'p2':1850003})
    appendLine({'t':'line' ,'p1':1850003 ,'p2':1850005})
    appendLine({'t':'line' ,'p1':1850005 ,'p2':1840004})
    appendLine({'t':'line' ,'p1':1850005 ,'p2':1830005})
    appendLine({'t':'line' ,'p1':1830005 ,'p2':1820005})
    appendLine({'t':'line' ,'p1':1820005 ,'p2':1810005})
    appendLine({'t':'line' ,'p1':1810005 ,'p2':1800004})
    appendLine({'t':'line' ,'p1':1860000 ,'p2':1880000})
    appendLine({'t':'line' ,'p1':1880000 ,'p2':1890000})
    appendLine({'t':'line' ,'p1':1890000 ,'p2':1900000})
    appendLine({'t':'line' ,'p1':1860000 ,'p2':1860002})
    appendLine({'t':'line' ,'p1':1860002 ,'p2':1860003})
    appendLine({'t':'line' ,'p1':1860003 ,'p2':1860004})
    appendLine({'t':'line' ,'p1':1860004 ,'p2':1900004})
    appendLine({'t':'line' ,'p1':1900004 ,'p2':1900000})
    appendLine({'t':'line' ,'p1':1900000 ,'p2':1910001})
    appendLine({'t':'line' ,'p1':1910001 ,'p2':1910002})
    appendLine({'t':'line' ,'p1':1910002 ,'p2':1910003})
    appendLine({'t':'line' ,'p1':1910003 ,'p2':1910005})
    appendLine({'t':'line' ,'p1':1910005 ,'p2':1900004})
    appendLine({'t':'line' ,'p1':1910005 ,'p2':1890005})
    appendLine({'t':'line' ,'p1':1890005 ,'p2':1880005})
    appendLine({'t':'line' ,'p1':1880005 ,'p2':1870005})
    appendLine({'t':'line' ,'p1':1870005 ,'p2':1860004})
    appendLine({'t':'line' ,'p1':1800006 ,'p2':1820006})
    appendLine({'t':'line' ,'p1':1820006 ,'p2':1830006})
    appendLine({'t':'line' ,'p1':1830006 ,'p2':1840006})
    appendLine({'t':'line' ,'p1':1800006 ,'p2':1800008})
    appendLine({'t':'line' ,'p1':1800008 ,'p2':1800009})
    appendLine({'t':'line' ,'p1':1800009 ,'p2':1800010})
    appendLine({'t':'line' ,'p1':1800010 ,'p2':1840010})
    appendLine({'t':'line' ,'p1':1840010 ,'p2':1840006})
    appendLine({'t':'line' ,'p1':1840006 ,'p2':1850007})
    appendLine({'t':'line' ,'p1':1850007 ,'p2':1850008})
    appendLine({'t':'line' ,'p1':1850008 ,'p2':1850009})
    appendLine({'t':'line' ,'p1':1850009 ,'p2':1850011})
    appendLine({'t':'line' ,'p1':1850011 ,'p2':1840010})
    appendLine({'t':'line' ,'p1':1850011 ,'p2':1830011})
    appendLine({'t':'line' ,'p1':1830011 ,'p2':1820011})
    appendLine({'t':'line' ,'p1':1820011 ,'p2':1810011})
    appendLine({'t':'line' ,'p1':1810011 ,'p2':1800010})
    appendLine({'t':'line' ,'p1':1860006 ,'p2':1880006})
    appendLine({'t':'line' ,'p1':1880006 ,'p2':1890006})
    appendLine({'t':'line' ,'p1':1890006 ,'p2':1900006})
    appendLine({'t':'line' ,'p1':1860006 ,'p2':1860008})
    appendLine({'t':'line' ,'p1':1860008 ,'p2':1860009})
    appendLine({'t':'line' ,'p1':1860009 ,'p2':1860010})
    appendLine({'t':'line' ,'p1':1860010 ,'p2':1900010})
    appendLine({'t':'line' ,'p1':1900010 ,'p2':1900006})
    appendLine({'t':'line' ,'p1':1900006 ,'p2':1910007})
    appendLine({'t':'line' ,'p1':1910007 ,'p2':1910008})
    appendLine({'t':'line' ,'p1':1910008 ,'p2':1910009})
    appendLine({'t':'line' ,'p1':1910009 ,'p2':1910011})
    appendLine({'t':'line' ,'p1':1910011 ,'p2':1900010})
    appendLine({'t':'line' ,'p1':1910011 ,'p2':1890011})
    appendLine({'t':'line' ,'p1':1890011 ,'p2':1880011})
    appendLine({'t':'line' ,'p1':1880011 ,'p2':1870011})
    appendLine({'t':'line' ,'p1':1870011 ,'p2':1860010})
    appendLine({'t':'line' ,'p1':1790003 ,'p2':1800003})
    appendLine({'t':'line' ,'p1':1770005 ,'p2':1770006})
    appendLine({'t':'line' ,'p1':1820005 ,'p2':1820006})
    appendLine({'t':'line' ,'p1':1790008 ,'p2':1800008})
    appendLine({'t':'line' ,'p1':1970003 ,'p2':1980003})
    appendLine({'t':'line' ,'p1':1950005 ,'p2':1950006})
    appendLine({'t':'line' ,'p1':2000005 ,'p2':2000006})
    appendLine({'t':'line' ,'p1':1970008 ,'p2':1980008})
    appendLine({'t':'line' ,'p1':1920000 ,'p2':1940000})
    appendLine({'t':'line' ,'p1':1940000 ,'p2':1950000})
    appendLine({'t':'line' ,'p1':1950000 ,'p2':1960000})
    appendLine({'t':'line' ,'p1':1920000 ,'p2':1920002})
    appendLine({'t':'line' ,'p1':1920002 ,'p2':1920003})
    appendLine({'t':'line' ,'p1':1920003 ,'p2':1920004})
    appendLine({'t':'line' ,'p1':1920004 ,'p2':1960004})
    appendLine({'t':'line' ,'p1':1960004 ,'p2':1960000})
    appendLine({'t':'line' ,'p1':1960000 ,'p2':1970001})
    appendLine({'t':'line' ,'p1':1970001 ,'p2':1970002})
    appendLine({'t':'line' ,'p1':1970002 ,'p2':1970003})
    appendLine({'t':'line' ,'p1':1970003 ,'p2':1970005})
    appendLine({'t':'line' ,'p1':1970005 ,'p2':1960004})
    appendLine({'t':'line' ,'p1':1970005 ,'p2':1950005})
    appendLine({'t':'line' ,'p1':1950005 ,'p2':1940005})
    appendLine({'t':'line' ,'p1':1940005 ,'p2':1930005})
    appendLine({'t':'line' ,'p1':1930005 ,'p2':1920004})
    appendLine({'t':'line' ,'p1':1980000 ,'p2':2000000})
    appendLine({'t':'line' ,'p1':2000000 ,'p2':2010000})
    appendLine({'t':'line' ,'p1':2010000 ,'p2':2020000})
    appendLine({'t':'line' ,'p1':1980000 ,'p2':1980002})
    appendLine({'t':'line' ,'p1':1980002 ,'p2':1980003})
    appendLine({'t':'line' ,'p1':1980003 ,'p2':1980004})
    appendLine({'t':'line' ,'p1':1980004 ,'p2':2020004})
    appendLine({'t':'line' ,'p1':2020004 ,'p2':2020000})
    appendLine({'t':'line' ,'p1':2020000 ,'p2':2030001})
    appendLine({'t':'line' ,'p1':2030001 ,'p2':2030002})
    appendLine({'t':'line' ,'p1':2030002 ,'p2':2030003})
    appendLine({'t':'line' ,'p1':2030003 ,'p2':2030005})
    appendLine({'t':'line' ,'p1':2030005 ,'p2':2020004})
    appendLine({'t':'line' ,'p1':2030005 ,'p2':2010005})
    appendLine({'t':'line' ,'p1':2010005 ,'p2':2000005})
    appendLine({'t':'line' ,'p1':2000005 ,'p2':1990005})
    appendLine({'t':'line' ,'p1':1990005 ,'p2':1980004})
    appendLine({'t':'line' ,'p1':1920006 ,'p2':1940006})
    appendLine({'t':'line' ,'p1':1940006 ,'p2':1950006})
    appendLine({'t':'line' ,'p1':1950006 ,'p2':1960006})
    appendLine({'t':'line' ,'p1':1920006 ,'p2':1920008})
    appendLine({'t':'line' ,'p1':1920008 ,'p2':1920009})
    appendLine({'t':'line' ,'p1':1920009 ,'p2':1920010})
    appendLine({'t':'line' ,'p1':1920010 ,'p2':1960010})
    appendLine({'t':'line' ,'p1':1960010 ,'p2':1960006})
    appendLine({'t':'line' ,'p1':1960006 ,'p2':1970007})
    appendLine({'t':'line' ,'p1':1970007 ,'p2':1970008})
    appendLine({'t':'line' ,'p1':1970008 ,'p2':1970009})
    appendLine({'t':'line' ,'p1':1970009 ,'p2':1970011})
    appendLine({'t':'line' ,'p1':1970011 ,'p2':1960010})
    appendLine({'t':'line' ,'p1':1970011 ,'p2':1950011})
    appendLine({'t':'line' ,'p1':1950011 ,'p2':1940011})
    appendLine({'t':'line' ,'p1':1940011 ,'p2':1930011})
    appendLine({'t':'line' ,'p1':1930011 ,'p2':1920010})
    appendLine({'t':'line' ,'p1':1980006 ,'p2':2000006})
    appendLine({'t':'line' ,'p1':2000006 ,'p2':2010006})
    appendLine({'t':'line' ,'p1':2010006 ,'p2':2020006})
    appendLine({'t':'line' ,'p1':1980006 ,'p2':1980008})
    appendLine({'t':'line' ,'p1':1980008 ,'p2':1980009})
    appendLine({'t':'line' ,'p1':1980009 ,'p2':1980010})
    appendLine({'t':'line' ,'p1':1980010 ,'p2':2020010})
    appendLine({'t':'line' ,'p1':2020010 ,'p2':2020006})
    appendLine({'t':'line' ,'p1':2020006 ,'p2':2030007})
    appendLine({'t':'line' ,'p1':2030007 ,'p2':2030008})
    appendLine({'t':'line' ,'p1':2030008 ,'p2':2030009})
    appendLine({'t':'line' ,'p1':2030009 ,'p2':2030011})
    appendLine({'t':'line' ,'p1':2030011 ,'p2':2020010})
    appendLine({'t':'line' ,'p1':2030011 ,'p2':2010011})
    appendLine({'t':'line' ,'p1':2010011 ,'p2':2000011})
    appendLine({'t':'line' ,'p1':2000011 ,'p2':1990011})
    appendLine({'t':'line' ,'p1':1990011 ,'p2':1980010})
    appendLine({'t':'line' ,'p1':1910003 ,'p2':1920003})
    appendLine({'t':'line' ,'p1':1890005 ,'p2':1890006})
    appendLine({'t':'line' ,'p1':1940005 ,'p2':1940006})
    appendLine({'t':'line' ,'p1':1910008 ,'p2':1920008})

        //     appendLine({'t':'line' ,'p1':50003 ,'p2':60003})
        // appendLine({'t':'line' ,'p1':30005 ,'p2':30006})
        // appendLine({'t':'line' ,'p1':80005 ,'p2':80006})
        // appendLine({'t':'line' ,'p1':50008 ,'p2':60008})
        // appendLine({'t':'line' ,'p1':0 ,'p2':20000})
        // appendLine({'t':'line' ,'p1':20000 ,'p2':30000})
        // appendLine({'t':'line' ,'p1':30000 ,'p2':40000})
        // appendLine({'t':'line' ,'p1':0 ,'p2':2})
        // appendLine({'t':'line' ,'p1':2 ,'p2':3})
        // appendLine({'t':'line' ,'p1':3 ,'p2':4})
        // appendLine({'t':'line' ,'p1':4 ,'p2':40004})
        // appendLine({'t':'line' ,'p1':40004 ,'p2':40000})
        // appendLine({'t':'line' ,'p1':40000 ,'p2':50001})
        // appendLine({'t':'line' ,'p1':50001 ,'p2':50002})
        // appendLine({'t':'line' ,'p1':50002 ,'p2':50003})
        // appendLine({'t':'line' ,'p1':50003 ,'p2':50005})
        // appendLine({'t':'line' ,'p1':50005 ,'p2':40004})
        // appendLine({'t':'line' ,'p1':50005 ,'p2':30005})
        // appendLine({'t':'line' ,'p1':30005 ,'p2':20005})
        // appendLine({'t':'line' ,'p1':20005 ,'p2':10005})
        // appendLine({'t':'line' ,'p1':10005 ,'p2':4})
        // appendLine({'t':'line' ,'p1':60000 ,'p2':80000})
        // appendLine({'t':'line' ,'p1':80000 ,'p2':90000})
        // appendLine({'t':'line' ,'p1':90000 ,'p2':100000})
        // appendLine({'t':'line' ,'p1':60000 ,'p2':60002})
        // appendLine({'t':'line' ,'p1':60002 ,'p2':60003})
        // appendLine({'t':'line' ,'p1':60003 ,'p2':60004})
        // appendLine({'t':'line' ,'p1':60004 ,'p2':100004})
        // appendLine({'t':'line' ,'p1':100004 ,'p2':100000})
        // appendLine({'t':'line' ,'p1':100000 ,'p2':110001})
        // appendLine({'t':'line' ,'p1':110001 ,'p2':110002})
        // appendLine({'t':'line' ,'p1':110002 ,'p2':110003})
        // appendLine({'t':'line' ,'p1':110003 ,'p2':110005})
        // appendLine({'t':'line' ,'p1':110005 ,'p2':100004})
        // appendLine({'t':'line' ,'p1':110005 ,'p2':90005})
        // appendLine({'t':'line' ,'p1':90005 ,'p2':80005})
        // appendLine({'t':'line' ,'p1':80005 ,'p2':70005})
        // appendLine({'t':'line' ,'p1':70005 ,'p2':60004})
        // appendLine({'t':'line' ,'p1':6 ,'p2':20006})
        // appendLine({'t':'line' ,'p1':20006 ,'p2':30006})
        // appendLine({'t':'line' ,'p1':30006 ,'p2':40006})
        // appendLine({'t':'line' ,'p1':6 ,'p2':8})
        // appendLine({'t':'line' ,'p1':8 ,'p2':9})
        // appendLine({'t':'line' ,'p1':9 ,'p2':10})
        // appendLine({'t':'line' ,'p1':10 ,'p2':40010})
        // appendLine({'t':'line' ,'p1':40010 ,'p2':40006})
        // appendLine({'t':'line' ,'p1':40006 ,'p2':50007})
        // appendLine({'t':'line' ,'p1':50007 ,'p2':50008})
        // appendLine({'t':'line' ,'p1':50008 ,'p2':50009})
        // appendLine({'t':'line' ,'p1':50009 ,'p2':50011})
        // appendLine({'t':'line' ,'p1':50011 ,'p2':40010})
        // appendLine({'t':'line' ,'p1':50011 ,'p2':30011})
        // appendLine({'t':'line' ,'p1':30011 ,'p2':20011})
        // appendLine({'t':'line' ,'p1':20011 ,'p2':10011})
        // appendLine({'t':'line' ,'p1':10011 ,'p2':10})
        // appendLine({'t':'line' ,'p1':60006 ,'p2':80006})
        // appendLine({'t':'line' ,'p1':80006 ,'p2':90006})
        // appendLine({'t':'line' ,'p1':90006 ,'p2':100006})
        // appendLine({'t':'line' ,'p1':60006 ,'p2':60008})
        // appendLine({'t':'line' ,'p1':60008 ,'p2':60009})
        // appendLine({'t':'line' ,'p1':60009 ,'p2':60010})
        // appendLine({'t':'line' ,'p1':60010 ,'p2':100010})
        // appendLine({'t':'line' ,'p1':100010 ,'p2':100006})
        // appendLine({'t':'line' ,'p1':100006 ,'p2':110007})
        // appendLine({'t':'line' ,'p1':110007 ,'p2':110008})
        // appendLine({'t':'line' ,'p1':110008 ,'p2':110009})
        // appendLine({'t':'line' ,'p1':110009 ,'p2':110011})
        // appendLine({'t':'line' ,'p1':110011 ,'p2':100010})
        // appendLine({'t':'line' ,'p1':110011 ,'p2':90011})
        // appendLine({'t':'line' ,'p1':90011 ,'p2':80011})
        // appendLine({'t':'line' ,'p1':80011 ,'p2':70011})
        // appendLine({'t':'line' ,'p1':70011 ,'p2':60010})
        // appendLine({'t':'line' ,'p1':170003 ,'p2':180003})
        // appendLine({'t':'line' ,'p1':150005 ,'p2':150006})
        // appendLine({'t':'line' ,'p1':200005 ,'p2':200006})
        // appendLine({'t':'line' ,'p1':170008 ,'p2':180008})
        // appendLine({'t':'line' ,'p1':120000 ,'p2':140000})
        // appendLine({'t':'line' ,'p1':140000 ,'p2':150000})
        // appendLine({'t':'line' ,'p1':150000 ,'p2':160000})
        // appendLine({'t':'line' ,'p1':120000 ,'p2':120002})
        // appendLine({'t':'line' ,'p1':120002 ,'p2':120003})
        // appendLine({'t':'line' ,'p1':120003 ,'p2':120004})
        // appendLine({'t':'line' ,'p1':120004 ,'p2':160004})
        // appendLine({'t':'line' ,'p1':160004 ,'p2':160000})
        // appendLine({'t':'line' ,'p1':160000 ,'p2':170001})
        // appendLine({'t':'line' ,'p1':170001 ,'p2':170002})
        // appendLine({'t':'line' ,'p1':170002 ,'p2':170003})
        // appendLine({'t':'line' ,'p1':170003 ,'p2':170005})
        // appendLine({'t':'line' ,'p1':170005 ,'p2':160004})
        // appendLine({'t':'line' ,'p1':170005 ,'p2':150005})
        // appendLine({'t':'line' ,'p1':150005 ,'p2':140005})
        // appendLine({'t':'line' ,'p1':140005 ,'p2':130005})
        // appendLine({'t':'line' ,'p1':130005 ,'p2':120004})
        // appendLine({'t':'line' ,'p1':180000 ,'p2':200000})
        // appendLine({'t':'line' ,'p1':200000 ,'p2':210000})
        // appendLine({'t':'line' ,'p1':210000 ,'p2':220000})
        // appendLine({'t':'line' ,'p1':180000 ,'p2':180002})
        // appendLine({'t':'line' ,'p1':180002 ,'p2':180003})
        // appendLine({'t':'line' ,'p1':180003 ,'p2':180004})
        // appendLine({'t':'line' ,'p1':180004 ,'p2':220004})
        // appendLine({'t':'line' ,'p1':220004 ,'p2':220000})
        // appendLine({'t':'line' ,'p1':220000 ,'p2':230001})
        // appendLine({'t':'line' ,'p1':230001 ,'p2':230002})
        // appendLine({'t':'line' ,'p1':230002 ,'p2':230003})
        // appendLine({'t':'line' ,'p1':230003 ,'p2':230005})
        // appendLine({'t':'line' ,'p1':230005 ,'p2':220004})
        // appendLine({'t':'line' ,'p1':230005 ,'p2':210005})
        // appendLine({'t':'line' ,'p1':210005 ,'p2':200005})
        // appendLine({'t':'line' ,'p1':200005 ,'p2':190005})
        // appendLine({'t':'line' ,'p1':190005 ,'p2':180004})
        // appendLine({'t':'line' ,'p1':120006 ,'p2':140006})
        // appendLine({'t':'line' ,'p1':140006 ,'p2':150006})
        // appendLine({'t':'line' ,'p1':150006 ,'p2':160006})
        // appendLine({'t':'line' ,'p1':120006 ,'p2':120008})
        // appendLine({'t':'line' ,'p1':120008 ,'p2':120009})
        // appendLine({'t':'line' ,'p1':120009 ,'p2':120010})
        // appendLine({'t':'line' ,'p1':120010 ,'p2':160010})
        // appendLine({'t':'line' ,'p1':160010 ,'p2':160006})
        // appendLine({'t':'line' ,'p1':160006 ,'p2':170007})
        // appendLine({'t':'line' ,'p1':170007 ,'p2':170008})
        // appendLine({'t':'line' ,'p1':170008 ,'p2':170009})
        // appendLine({'t':'line' ,'p1':170009 ,'p2':170011})
        // appendLine({'t':'line' ,'p1':170011 ,'p2':160010})
        // appendLine({'t':'line' ,'p1':170011 ,'p2':150011})
        // appendLine({'t':'line' ,'p1':150011 ,'p2':140011})
        // appendLine({'t':'line' ,'p1':140011 ,'p2':130011})
        // appendLine({'t':'line' ,'p1':130011 ,'p2':120010})
        // appendLine({'t':'line' ,'p1':180006 ,'p2':200006})
        // appendLine({'t':'line' ,'p1':200006 ,'p2':210006})
        // appendLine({'t':'line' ,'p1':210006 ,'p2':220006})
        // appendLine({'t':'line' ,'p1':180006 ,'p2':180008})
        // appendLine({'t':'line' ,'p1':180008 ,'p2':180009})
        // appendLine({'t':'line' ,'p1':180009 ,'p2':180010})
        // appendLine({'t':'line' ,'p1':180010 ,'p2':220010})
        // appendLine({'t':'line' ,'p1':220010 ,'p2':220006})
        // appendLine({'t':'line' ,'p1':220006 ,'p2':230007})
        // appendLine({'t':'line' ,'p1':230007 ,'p2':230008})
        // appendLine({'t':'line' ,'p1':230008 ,'p2':230009})
        // appendLine({'t':'line' ,'p1':230009 ,'p2':230011})
        // appendLine({'t':'line' ,'p1':230011 ,'p2':220010})
        // appendLine({'t':'line' ,'p1':230011 ,'p2':210011})
        // appendLine({'t':'line' ,'p1':210011 ,'p2':200011})
        // appendLine({'t':'line' ,'p1':200011 ,'p2':190011})
        // appendLine({'t':'line' ,'p1':190011 ,'p2':180010})
        // appendLine({'t':'line' ,'p1':110003 ,'p2':120003})
        // appendLine({'t':'line' ,'p1':90005 ,'p2':90006})
        // appendLine({'t':'line' ,'p1':140005 ,'p2':140006})
        // appendLine({'t':'line' ,'p1':110008 ,'p2':120008})
        // appendLine({'t':'line' ,'p1':290003 ,'p2':300003})
        // appendLine({'t':'line' ,'p1':270005 ,'p2':270006})
        // appendLine({'t':'line' ,'p1':320005 ,'p2':320006})
        // appendLine({'t':'line' ,'p1':290008 ,'p2':300008})
        // appendLine({'t':'line' ,'p1':240000 ,'p2':260000})
        // appendLine({'t':'line' ,'p1':260000 ,'p2':270000})
        // appendLine({'t':'line' ,'p1':270000 ,'p2':280000})
        // appendLine({'t':'line' ,'p1':240000 ,'p2':240002})
        // appendLine({'t':'line' ,'p1':240002 ,'p2':240003})
        // appendLine({'t':'line' ,'p1':240003 ,'p2':240004})
        // appendLine({'t':'line' ,'p1':240004 ,'p2':280004})
        // appendLine({'t':'line' ,'p1':280004 ,'p2':280000})
        // appendLine({'t':'line' ,'p1':280000 ,'p2':290001})
        // appendLine({'t':'line' ,'p1':290001 ,'p2':290002})
        // appendLine({'t':'line' ,'p1':290002 ,'p2':290003})
        // appendLine({'t':'line' ,'p1':290003 ,'p2':290005})
        // appendLine({'t':'line' ,'p1':290005 ,'p2':280004})
        // appendLine({'t':'line' ,'p1':290005 ,'p2':270005})
        // appendLine({'t':'line' ,'p1':270005 ,'p2':260005})
        // appendLine({'t':'line' ,'p1':260005 ,'p2':250005})
        // appendLine({'t':'line' ,'p1':250005 ,'p2':240004})
        // appendLine({'t':'line' ,'p1':300000 ,'p2':320000})
        // appendLine({'t':'line' ,'p1':320000 ,'p2':330000})
        // appendLine({'t':'line' ,'p1':330000 ,'p2':340000})
        // appendLine({'t':'line' ,'p1':300000 ,'p2':300002})
        // appendLine({'t':'line' ,'p1':300002 ,'p2':300003})
        // appendLine({'t':'line' ,'p1':300003 ,'p2':300004})
        // appendLine({'t':'line' ,'p1':300004 ,'p2':340004})
        // appendLine({'t':'line' ,'p1':340004 ,'p2':340000})
        // appendLine({'t':'line' ,'p1':340000 ,'p2':350001})
        // appendLine({'t':'line' ,'p1':350001 ,'p2':350002})
        // appendLine({'t':'line' ,'p1':350002 ,'p2':350003})
        // appendLine({'t':'line' ,'p1':350003 ,'p2':350005})
        // appendLine({'t':'line' ,'p1':350005 ,'p2':340004})
        // appendLine({'t':'line' ,'p1':350005 ,'p2':330005})
        // appendLine({'t':'line' ,'p1':330005 ,'p2':320005})
        // appendLine({'t':'line' ,'p1':320005 ,'p2':310005})
        // appendLine({'t':'line' ,'p1':310005 ,'p2':300004})
        // appendLine({'t':'line' ,'p1':240006 ,'p2':260006})
        // appendLine({'t':'line' ,'p1':260006 ,'p2':270006})
        // appendLine({'t':'line' ,'p1':270006 ,'p2':280006})
        // appendLine({'t':'line' ,'p1':240006 ,'p2':240008})
        // appendLine({'t':'line' ,'p1':240008 ,'p2':240009})
        // appendLine({'t':'line' ,'p1':240009 ,'p2':240010})
        // appendLine({'t':'line' ,'p1':240010 ,'p2':280010})
        // appendLine({'t':'line' ,'p1':280010 ,'p2':280006})
        // appendLine({'t':'line' ,'p1':280006 ,'p2':290007})
        // appendLine({'t':'line' ,'p1':290007 ,'p2':290008})
        // appendLine({'t':'line' ,'p1':290008 ,'p2':290009})
        // appendLine({'t':'line' ,'p1':290009 ,'p2':290011})
        // appendLine({'t':'line' ,'p1':290011 ,'p2':280010})
        // appendLine({'t':'line' ,'p1':290011 ,'p2':270011})
        // appendLine({'t':'line' ,'p1':270011 ,'p2':260011})
        // appendLine({'t':'line' ,'p1':260011 ,'p2':250011})
        // appendLine({'t':'line' ,'p1':250011 ,'p2':240010})
        // appendLine({'t':'line' ,'p1':300006 ,'p2':320006})
        // appendLine({'t':'line' ,'p1':320006 ,'p2':330006})
        // appendLine({'t':'line' ,'p1':330006 ,'p2':340006})
        // appendLine({'t':'line' ,'p1':300006 ,'p2':300008})
        // appendLine({'t':'line' ,'p1':300008 ,'p2':300009})
        // appendLine({'t':'line' ,'p1':300009 ,'p2':300010})
        // appendLine({'t':'line' ,'p1':300010 ,'p2':340010})
        // appendLine({'t':'line' ,'p1':340010 ,'p2':340006})
        // appendLine({'t':'line' ,'p1':340006 ,'p2':350007})
        // appendLine({'t':'line' ,'p1':350007 ,'p2':350008})
        // appendLine({'t':'line' ,'p1':350008 ,'p2':350009})
        // appendLine({'t':'line' ,'p1':350009 ,'p2':350011})
        // appendLine({'t':'line' ,'p1':350011 ,'p2':340010})
        // appendLine({'t':'line' ,'p1':350011 ,'p2':330011})
        // appendLine({'t':'line' ,'p1':330011 ,'p2':320011})
        // appendLine({'t':'line' ,'p1':320011 ,'p2':310011})
        // appendLine({'t':'line' ,'p1':310011 ,'p2':300010})
        // appendLine({'t':'line' ,'p1':230003 ,'p2':240003})
        // appendLine({'t':'line' ,'p1':210005 ,'p2':210006})
        // appendLine({'t':'line' ,'p1':260005 ,'p2':260006})
        // appendLine({'t':'line' ,'p1':230008 ,'p2':240008})
        // appendLine({'t':'line' ,'p1':410003 ,'p2':420003})
        // appendLine({'t':'line' ,'p1':390005 ,'p2':390006})
        // appendLine({'t':'line' ,'p1':440005 ,'p2':440006})
        // appendLine({'t':'line' ,'p1':410008 ,'p2':420008})
        // appendLine({'t':'line' ,'p1':360000 ,'p2':380000})
        // appendLine({'t':'line' ,'p1':380000 ,'p2':390000})
        // appendLine({'t':'line' ,'p1':390000 ,'p2':400000})
        // appendLine({'t':'line' ,'p1':360000 ,'p2':360002})
        // appendLine({'t':'line' ,'p1':360002 ,'p2':360003})
        // appendLine({'t':'line' ,'p1':360003 ,'p2':360004})
        // appendLine({'t':'line' ,'p1':360004 ,'p2':400004})
        // appendLine({'t':'line' ,'p1':400004 ,'p2':400000})
        // appendLine({'t':'line' ,'p1':400000 ,'p2':410001})
        // appendLine({'t':'line' ,'p1':410001 ,'p2':410002})
        // appendLine({'t':'line' ,'p1':410002 ,'p2':410003})
        // appendLine({'t':'line' ,'p1':410003 ,'p2':410005})
        // appendLine({'t':'line' ,'p1':410005 ,'p2':400004})
        // appendLine({'t':'line' ,'p1':410005 ,'p2':390005})
        // appendLine({'t':'line' ,'p1':390005 ,'p2':380005})
        // appendLine({'t':'line' ,'p1':380005 ,'p2':370005})
        // appendLine({'t':'line' ,'p1':370005 ,'p2':360004})
        // appendLine({'t':'line' ,'p1':420000 ,'p2':440000})
        // appendLine({'t':'line' ,'p1':440000 ,'p2':450000})
        // appendLine({'t':'line' ,'p1':450000 ,'p2':460000})
        // appendLine({'t':'line' ,'p1':420000 ,'p2':420002})
        // appendLine({'t':'line' ,'p1':420002 ,'p2':420003})
        // appendLine({'t':'line' ,'p1':420003 ,'p2':420004})
        // appendLine({'t':'line' ,'p1':420004 ,'p2':460004})
        // appendLine({'t':'line' ,'p1':460004 ,'p2':460000})
        // appendLine({'t':'line' ,'p1':460000 ,'p2':470001})
        // appendLine({'t':'line' ,'p1':470001 ,'p2':470002})
        // appendLine({'t':'line' ,'p1':470002 ,'p2':470003})
        // appendLine({'t':'line' ,'p1':470003 ,'p2':470005})
        // appendLine({'t':'line' ,'p1':470005 ,'p2':460004})
        // appendLine({'t':'line' ,'p1':470005 ,'p2':450005})
        // appendLine({'t':'line' ,'p1':450005 ,'p2':440005})
        // appendLine({'t':'line' ,'p1':440005 ,'p2':430005})
        // appendLine({'t':'line' ,'p1':430005 ,'p2':420004})
        // appendLine({'t':'line' ,'p1':360006 ,'p2':380006})
        // appendLine({'t':'line' ,'p1':380006 ,'p2':390006})
        // appendLine({'t':'line' ,'p1':390006 ,'p2':400006})
        // appendLine({'t':'line' ,'p1':360006 ,'p2':360008})
        // appendLine({'t':'line' ,'p1':360008 ,'p2':360009})
        // appendLine({'t':'line' ,'p1':360009 ,'p2':360010})
        // appendLine({'t':'line' ,'p1':360010 ,'p2':400010})
        // appendLine({'t':'line' ,'p1':400010 ,'p2':400006})
        // appendLine({'t':'line' ,'p1':400006 ,'p2':410007})
        // appendLine({'t':'line' ,'p1':410007 ,'p2':410008})
        // appendLine({'t':'line' ,'p1':410008 ,'p2':410009})
        // appendLine({'t':'line' ,'p1':410009 ,'p2':410011})
        // appendLine({'t':'line' ,'p1':410011 ,'p2':400010})
        // appendLine({'t':'line' ,'p1':410011 ,'p2':390011})
        // appendLine({'t':'line' ,'p1':390011 ,'p2':380011})
        // appendLine({'t':'line' ,'p1':380011 ,'p2':370011})
        // appendLine({'t':'line' ,'p1':370011 ,'p2':360010})
        // appendLine({'t':'line' ,'p1':420006 ,'p2':440006})
        // appendLine({'t':'line' ,'p1':440006 ,'p2':450006})
        // appendLine({'t':'line' ,'p1':450006 ,'p2':460006})
        // appendLine({'t':'line' ,'p1':420006 ,'p2':420008})
        // appendLine({'t':'line' ,'p1':420008 ,'p2':420009})
        // appendLine({'t':'line' ,'p1':420009 ,'p2':420010})
        // appendLine({'t':'line' ,'p1':420010 ,'p2':460010})
        // appendLine({'t':'line' ,'p1':460010 ,'p2':460006})
        // appendLine({'t':'line' ,'p1':460006 ,'p2':470007})
        // appendLine({'t':'line' ,'p1':470007 ,'p2':470008})
        // appendLine({'t':'line' ,'p1':470008 ,'p2':470009})
        // appendLine({'t':'line' ,'p1':470009 ,'p2':470011})
        // appendLine({'t':'line' ,'p1':470011 ,'p2':460010})
        // appendLine({'t':'line' ,'p1':470011 ,'p2':450011})
        // appendLine({'t':'line' ,'p1':450011 ,'p2':440011})
        // appendLine({'t':'line' ,'p1':440011 ,'p2':430011})
        // appendLine({'t':'line' ,'p1':430011 ,'p2':420010})
        // appendLine({'t':'line' ,'p1':350003 ,'p2':360003})
        // appendLine({'t':'line' ,'p1':330005 ,'p2':330006})
        // appendLine({'t':'line' ,'p1':380005 ,'p2':380006})
        // appendLine({'t':'line' ,'p1':350008 ,'p2':360008})
        // appendLine({'t':'line' ,'p1':530003 ,'p2':540003})
        // appendLine({'t':'line' ,'p1':510005 ,'p2':510006})
        // appendLine({'t':'line' ,'p1':560005 ,'p2':560006})
        // appendLine({'t':'line' ,'p1':530008 ,'p2':540008})
        // appendLine({'t':'line' ,'p1':480000 ,'p2':500000})
        // appendLine({'t':'line' ,'p1':500000 ,'p2':510000})
        // appendLine({'t':'line' ,'p1':510000 ,'p2':520000})
        // appendLine({'t':'line' ,'p1':480000 ,'p2':480002})
        // appendLine({'t':'line' ,'p1':480002 ,'p2':480003})
        // appendLine({'t':'line' ,'p1':480003 ,'p2':480004})
        // appendLine({'t':'line' ,'p1':480004 ,'p2':520004})
        // appendLine({'t':'line' ,'p1':520004 ,'p2':520000})
        // appendLine({'t':'line' ,'p1':520000 ,'p2':530001})
        // appendLine({'t':'line' ,'p1':530001 ,'p2':530002})
        // appendLine({'t':'line' ,'p1':530002 ,'p2':530003})
        // appendLine({'t':'line' ,'p1':530003 ,'p2':530005})
        // appendLine({'t':'line' ,'p1':530005 ,'p2':520004})
        // appendLine({'t':'line' ,'p1':530005 ,'p2':510005})
        // appendLine({'t':'line' ,'p1':510005 ,'p2':500005})
        // appendLine({'t':'line' ,'p1':500005 ,'p2':490005})
        // appendLine({'t':'line' ,'p1':490005 ,'p2':480004})
        // appendLine({'t':'line' ,'p1':540000 ,'p2':560000})
        // appendLine({'t':'line' ,'p1':560000 ,'p2':570000})
        // appendLine({'t':'line' ,'p1':570000 ,'p2':580000})
        // appendLine({'t':'line' ,'p1':540000 ,'p2':540002})
        // appendLine({'t':'line' ,'p1':540002 ,'p2':540003})
        // appendLine({'t':'line' ,'p1':540003 ,'p2':540004})
        // appendLine({'t':'line' ,'p1':540004 ,'p2':580004})
        // appendLine({'t':'line' ,'p1':580004 ,'p2':580000})
        // appendLine({'t':'line' ,'p1':580000 ,'p2':590001})
        // appendLine({'t':'line' ,'p1':590001 ,'p2':590002})
        // appendLine({'t':'line' ,'p1':590002 ,'p2':590003})
        // appendLine({'t':'line' ,'p1':590003 ,'p2':590005})
        // appendLine({'t':'line' ,'p1':590005 ,'p2':580004})
        // appendLine({'t':'line' ,'p1':590005 ,'p2':570005})
        // appendLine({'t':'line' ,'p1':570005 ,'p2':560005})
        // appendLine({'t':'line' ,'p1':560005 ,'p2':550005})
        // appendLine({'t':'line' ,'p1':550005 ,'p2':540004})
        // appendLine({'t':'line' ,'p1':480006 ,'p2':500006})
        // appendLine({'t':'line' ,'p1':500006 ,'p2':510006})
        // appendLine({'t':'line' ,'p1':510006 ,'p2':520006})
        // appendLine({'t':'line' ,'p1':480006 ,'p2':480008})
        // appendLine({'t':'line' ,'p1':480008 ,'p2':480009})
        // appendLine({'t':'line' ,'p1':480009 ,'p2':480010})
        // appendLine({'t':'line' ,'p1':480010 ,'p2':520010})
        // appendLine({'t':'line' ,'p1':520010 ,'p2':520006})
        // appendLine({'t':'line' ,'p1':520006 ,'p2':530007})
        // appendLine({'t':'line' ,'p1':530007 ,'p2':530008})
        // appendLine({'t':'line' ,'p1':530008 ,'p2':530009})
        // appendLine({'t':'line' ,'p1':530009 ,'p2':530011})
        // appendLine({'t':'line' ,'p1':530011 ,'p2':520010})
        // appendLine({'t':'line' ,'p1':530011 ,'p2':510011})
        // appendLine({'t':'line' ,'p1':510011 ,'p2':500011})
        // appendLine({'t':'line' ,'p1':500011 ,'p2':490011})
        // appendLine({'t':'line' ,'p1':490011 ,'p2':480010})
        // appendLine({'t':'line' ,'p1':540006 ,'p2':560006})
        // appendLine({'t':'line' ,'p1':560006 ,'p2':570006})
        // appendLine({'t':'line' ,'p1':570006 ,'p2':580006})
        // appendLine({'t':'line' ,'p1':540006 ,'p2':540008})
        // appendLine({'t':'line' ,'p1':540008 ,'p2':540009})
        // appendLine({'t':'line' ,'p1':540009 ,'p2':540010})
        // appendLine({'t':'line' ,'p1':540010 ,'p2':580010})
        // appendLine({'t':'line' ,'p1':580010 ,'p2':580006})
        // appendLine({'t':'line' ,'p1':580006 ,'p2':590007})
        // appendLine({'t':'line' ,'p1':590007 ,'p2':590008})
        // appendLine({'t':'line' ,'p1':590008 ,'p2':590009})
        // appendLine({'t':'line' ,'p1':590009 ,'p2':590011})
        // appendLine({'t':'line' ,'p1':590011 ,'p2':580010})
        // appendLine({'t':'line' ,'p1':590011 ,'p2':570011})
        // appendLine({'t':'line' ,'p1':570011 ,'p2':560011})
        // appendLine({'t':'line' ,'p1':560011 ,'p2':550011})
        // appendLine({'t':'line' ,'p1':550011 ,'p2':540010})
        // appendLine({'t':'line' ,'p1':470003 ,'p2':480003})
        // appendLine({'t':'line' ,'p1':450005 ,'p2':450006})
        // appendLine({'t':'line' ,'p1':500005 ,'p2':500006})
        // appendLine({'t':'line' ,'p1':470008 ,'p2':480008})

    state.onStateChanged(setCurrentState);
    return that;
}
