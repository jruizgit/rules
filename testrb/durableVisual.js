var r = 600;

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
    var rulesetUrl = url.substring(0, url.lastIndexOf('/')) + '/state';
    sid = '0';

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
        var statusUrl = rulesetUrl;
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
    var ruleSetUrl = url.substring(0, url.lastIndexOf('/')) + '/definition';
    sid = '0'

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

                nodes = getNodes(ruleSet, links);
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

                var title = ruleSetUrl.substring(1) + '/' + sid;
                if (nodes.type === 'stateChart') {
                    stateVisual(nodes, links, x, y, title, svg, state);
                } else if (nodes.type === 'flowChart') {
                    flowVisual(nodes, links, x, y, title, svg, state);
                } else {
                    rulesetVisual(nodes, links, x, y, title, svg, state);
                }

                state.start();
                if (callback) {
                    callback();
                }
            }
        });
    };

    state.onStateChanged(setCurrentState);
    return that;
}
