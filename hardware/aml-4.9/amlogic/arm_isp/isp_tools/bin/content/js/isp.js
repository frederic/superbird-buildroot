//----------------------------------------------------------------------------
// The confidential and proprietary information contained in this file may
// only be used by a person authorised under and to the extent permitted
// by a subsisting licensing agreement from ARM Limited or its affiliates.
//
//          (C) COPYRIGHT 2018 ARM Limited or its affiliates.
//                        ALL RIGHTS RESERVED
//
// This entire notice must be reproduced on all copies of this file
// and copies of this file may only be made by a person if such person is
// permitted to do so under the terms of a subsisting license agreementv
// from ARM Limited or its affiliates.
//----------------------------------------------------------------------------
const SVG = function() { 
    let that = this;
    const svgNS = 'http:
    const TYPE = {
        SVG: 'svg',
        DEFS: 'defs',
        GROUP: 'g',
        RECT: 'rect',
        CIRCLE: 'circle',
        POLYLINE: 'polyline',
        POLYGON: 'polygon',
        TEXT: 'text',
        FILTER: 'filter',
        BLUR: 'feGaussianBlur',
        TITLE: 'title',
        MERGE: 'feMerge',
        MERGE_NODE: 'feMergeNode'
    };
    const drawObject = function(type, attributes) {
        let obj = document.createElementNS(svgNS, type);
        for (let attributeName in attributes) {
            if (attributes.hasOwnProperty(attributeName)) {
                obj.setAttributeNS(null, attributeName, attributes[attributeName]);
            }
        }
        return obj;
    };
    that.svg = function(attributes) {
        return drawObject(TYPE.SVG, attributes);
    };
    that.group = function(attributes) {
        return drawObject(TYPE.GROUP, attributes);
    };
    that.rect = function(attributes) {
        return drawObject(TYPE.RECT, attributes);
    };
    that.circle = function(attributes) {
        return drawObject(TYPE.CIRCLE, attributes);
    };
    that.polyline = function(points, attributes) {
        let polyline = drawObject(TYPE.POLYLINE, attributes);
        let pointsString = '';
        for (let i = 0; i < points.length; i++) {
            pointsString += points[i][0] + ',' + points[i][1] + ' ';
        }
        polyline.setAttributeNS(null, 'points', pointsString);
        return polyline;
    };
    that.polygon = function(points, attributes) {
        let polygon = drawObject(TYPE.POLYGON, attributes);
        if (points) {
            let pointsString = '';
            for (let i = 0; i < points.length; i++) {
                pointsString += points[i][0] + ',' + points[i][1] + ' ';
            }
            polygon.setAttributeNS(null, 'points', pointsString);
        }
        return polygon;
    };
    that.text = function(text, attributes) {
        let textObject = drawObject(TYPE.TEXT, attributes);
        let textNode = document.createTextNode(text);
        textObject.appendChild(textNode);
        return textObject;
    };
    that.glow = function(id, deviation, x, y, width, height) {
        let defs = drawObject(TYPE.DEFS, {});
        let filter = drawObject(TYPE.FILTER, {id: id, x: x, y: y, width: width, height: height});
        let blur = drawObject(TYPE.BLUR, {stdDeviation: deviation, result: 'glow'});
        let merge = drawObject(TYPE.MERGE, {});
        let mergeNode1 = drawObject(TYPE.MERGE_NODE, {in: 'glow'});
        let mergeNode2 = drawObject(TYPE.MERGE_NODE, {in: 'glow'});
        let mergeNode3 = drawObject(TYPE.MERGE_NODE, {in: 'glow'});
        merge.appendChild(mergeNode1);
        merge.appendChild(mergeNode2);
        merge.appendChild(mergeNode3);
        filter.appendChild(blur);
        filter.appendChild(merge);
        defs.appendChild(filter);
        return defs;
    };
    that.title = function(titleText) {
        let title = drawObject(TYPE.TITLE);
        title.innerHTML = titleText;
        return title;
    };
};

const DrawView = function(elementID, view) { 
    let drawView = this;
    let element = document.getElementById(elementID);
    while (element.firstChild) {
        element.removeChild(element.firstChild);
    }

    drawView.width = element.offsetWidth;
    drawView.height = element.offsetHeight;

    const svg = new SVG();

    let masterSVG = svg.svg({id: 'masterSVG'});
    let dragSVG = svg.group({id: 'dragSVG'});
    let gridSVG = svg.svg({width: 10000, height: 10000});
    let connectionSVG = svg.svg({width: 10000, height: 10000});
    let multiConnectionSVG = svg.svg({width: 10000, height: 10000});
    let componentSVG = svg.svg({width: 10000, height: 10000, 'pointer-events': 'none'});
    let labelSVG = svg.svg({width: 10000, height: 10000});

    masterSVG.style.position = 'absolute';
    masterSVG.style.cursor = 'crosshair';
    gridSVG.style.position = 'absolute';
    connectionSVG.style.position = 'absolute';
    componentSVG.style.position = 'absolute';
    multiConnectionSVG.style.position = 'absolute';
    labelSVG.style.position = 'absolute';
    labelSVG.style['pointer-events'] = 'none';

    let glow = svg.glow('glow', '2 2', '-50%', '-50%', '200%', '200%');
    labelSVG.appendChild(glow);

    dragSVG.appendChild(gridSVG);
    dragSVG.appendChild(connectionSVG);
    dragSVG.appendChild(multiConnectionSVG);
    dragSVG.appendChild(componentSVG);
    dragSVG.appendChild(labelSVG);
    masterSVG.appendChild(dragSVG);

    for (let tile of view.tileSVGData) {
        let tileGraphic = svg.polygon(tile.points, tile.attributes);
        gridSVG.appendChild(tileGraphic);
    }

    for (let component of view.componentSVGData) {
        let componentGraphic = svg.polygon(component.block.points, component.block.attributes);
        componentSVG.appendChild(componentGraphic);
    }

    for (let component of view.componentSVGData) {
        let componentGraphic = svg.polygon(component.block.points, component.block.attributes);
        let componentTextGraphic = svg.text(component.text.text, component.text.attributes);
        var glowAttributes = component.text.attributes;
        glowAttributes['stroke'] = 'white';
        glowAttributes['fill'] = 'white';
        glowAttributes['opacity'] = '0.5';
        glowAttributes['style'] = 'filter: url(#glow)';
        let componentTextGraphicGlow = svg.text(component.text.text, glowAttributes);
        let componentInputLabelGraphic = svg.circle(component.inputLabel.attributes);
        let componentInputLabelTextGraphic = svg.text(component.inputLabelText.text, component.inputLabelText.attributes);
        componentSVG.appendChild(componentGraphic);
        labelSVG.appendChild(componentTextGraphicGlow);
        labelSVG.appendChild(componentTextGraphic);
        labelSVG.appendChild(componentInputLabelGraphic);
        labelSVG.appendChild(componentInputLabelTextGraphic);
    }

    for (let connection of view.connectionSVGData) {
        let lineGraphic = svg.polyline(connection.line.points, connection.line.attributes);
        let lineHighlightGraphic = svg.polyline(connection.highlight.points, connection.highlight.attributes);
        let lineMovingHighlightGraphic = svg.polyline(connection.movingHighlight.points, connection.movingHighlight.attributes);
        let lineHoverGraphic = svg.polyline(connection.hover.points, connection.hover.attributes);
        connectionSVG.appendChild(lineGraphic);
        connectionSVG.appendChild(lineHighlightGraphic);
        connectionSVG.appendChild(lineMovingHighlightGraphic);
        connectionSVG.appendChild(lineHoverGraphic);
    }

    for (let tile of view.multiConnectionTileSVGData) {
        let tileGraphic = svg.polygon(tile.points, tile.attributes);
        multiConnectionSVG.appendChild(tileGraphic);
    }

    for (let connection of view.borderGatewayConnectionSVGData) {
        let lineGraphic = svg.polyline(connection.line.points, connection.line.attributes);
        let lineHighlightGraphic = svg.polyline(connection.highlight.points, connection.highlight.attributes);
        let lineMovingHighlightGraphic = svg.polyline(connection.movingHighlight.points, connection.movingHighlight.attributes);
        let lineHoverGraphic = svg.polyline(connection.hover.points, connection.hover.attributes);
        multiConnectionSVG.appendChild(lineGraphic);
        multiConnectionSVG.appendChild(lineHighlightGraphic);
        multiConnectionSVG.appendChild(lineMovingHighlightGraphic);
        multiConnectionSVG.appendChild(lineHoverGraphic);
    }
    element.appendChild(masterSVG);
    let dragClientRect = dragSVG.getBoundingClientRect();
    let dragRect = dragSVG.getBBox();
    masterSVG.setAttribute('width', (dragClientRect.width + (dragRect.x * 0.3)));
    masterSVG.setAttribute('height', (dragClientRect.height + (dragRect.y * 0.3)));
};

const ParseComponentsAndInputs = function(xmlText) { 
    let componentsList = [];
    const parser = new DOMParser();
    let xmlDoc = parser.parseFromString(xmlText, "text/xml");
    let childrenListText;
    let childData;
    let linkedComponentsToInputIndex = new Map();
    for (let module of xmlDoc.getElementsByTagName('module')) {
        let childrenList = null;
        if (module.firstChild) {
            childrenListText = module.firstChild.nodeValue !== ''? module.firstChild.nodeValue.match(/\(([^)]+)\)/g): null;
            childrenList = [];
            if (childrenListText !== null) {
                for (let childText of childrenListText) {
                    childData = childText.slice(1, -1).split(',');
                    childrenList.push(childData[0]);
                    linkedComponentsToInputIndex.set(module.id + '_' + childData[0], childData[1]);
                }
            }
        }
        componentsList.push({id: module.id, name: module.getAttribute('name'), type: module.getAttribute('type').toLowerCase(), childrenList: childrenList});
    }
    return {componentsList: componentsList, linkedComponentsToInputIndex: linkedComponentsToInputIndex};
};
(function checkES6() {
    let a = 0; 
    document.body.removeChild(document.getElementById('es6-warning-header'));
})();

function asyncQueue(makeGenerator){
    return function () {
        let generator = makeGenerator.apply(this, arguments);

        const handle = function(result){
            if (result.done) {
                return Promise.resolve(result.value);
            }

            return Promise.resolve(result.value).then(function (res){
                return handle(generator.next(res));
            }, function (err){
                return handle(generator.throw(err));
            });
        };

        try {
            return handle(generator.next());
        } catch (ex) {
            return Promise.reject(ex);
        }
    };
}

const ISP = function(xmlFile, optionParameters) { 
    optionParameters = optionParameters || {};
    let parseComponentsListFromFile = function(file) {
        return new Promise(function(resolve) {
            let reader = new FileReader();
            reader.onload = function() {
                resolve(ParseComponentsAndInputs(reader.result));
            };
            reader.readAsText(file);
        });
    };

    let createView = function(components, linkedComponentsToInputIndex, options) {
        return new Promise(function(resolve) {
            let worker = new Worker('../js/ispw.js');
            worker.addEventListener('message', function (e) {
                if (e.data.type !== 'status') {
                    resolve(e.data);
                }
            }, false);
            worker.postMessage({components: components,
                linkedComponentsToInputIndex: linkedComponentsToInputIndex,
                options: options});
        });
    };

    let drawView = function(elementID, view) {
        return new Promise(function(resolve) {
            DrawView(elementID, view);
            resolve();
        });
    };

    let createISP = asyncQueue(function*(file, options) {
        let result = yield parseComponentsListFromFile(file);
        let view = yield createView(result.componentsList, result.linkedComponentsToInputIndex, options);
        yield drawView('drawing-area', view);
    });

    return createISP(xmlFile, optionParameters);

};