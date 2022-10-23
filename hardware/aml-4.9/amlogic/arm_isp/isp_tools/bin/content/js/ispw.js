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
const Node = function(data, cost) { 
    this.data = data;
    this.cost = cost;
};

const PriorityQueue = function() { 
    let that = this;
    let heap = [];
    that.push = function(x, cost) {
        let node = new Node(x, cost);
        if (heap.length === 0) {
            heap = [node];
        } else {
            for (let i = 0; i < heap.length; i++) {
                if (node.cost < heap[i].cost) {
                    heap.splice(i, 0, node);
                    return;
                }
            }
            heap.push(node);
        }
    };
    that.next = function() {
        return heap.shift();
    };
    that.last = function() {
        return heap.pop();
    };
    that.size = function() {
        return heap.length;
    };
};

/*
 * Components is a class for creating and housing instances of the Component class and related methods
 */

const Components = function(componentsList) { 
    let components = this;

    const Component = function(id, name, type) {
        let component = this;
        component.id = id;
        component.name = name;
        component.type = type;
        component.layer = -1;
    };

    components.componentsMap = new Map(); 
    components.parentToChildrenMap = new Map(); 
    components.childToParentsMap = new Map(); 
    components.roots = new Set(); 
    components.cyclicLinks = new Map(); 

    
    let createComponent = function(componentID, componentName, componentType, childrenIDList) {
        if (!components.componentsMap.has(componentID)) {
            let component = new Component(componentID, componentName, componentType);
            components.componentsMap.set(componentID, component);
            childrenIDList = childrenIDList || [];

            if (childrenIDList.length === 0 || childrenIDList === 'none') {
                components.parentToChildrenMap.set(componentID, 'none');
                components.roots.add(componentID);
            } else {
                components.parentToChildrenMap.set(componentID, childrenIDList);
                for (let i = 0; i < childrenIDList.length; i++) {
                    if (components.childToParentsMap.has(childrenIDList[i])) {
                        let parents = components.childToParentsMap.get(childrenIDList[i]);
                        parents.push(componentID);
                        components.childToParentsMap.set(childrenIDList[i], parents);
                    } else {
                        components.childToParentsMap.set(childrenIDList[i], [componentID]);
                    }
                }
            }
        }

    };

    for (let componentData of componentsList) {
        createComponent(componentData.id, componentData.name, componentData.type, componentData.childrenList);
    }

};
const MakeAcyclic = function(components) { 
    const breakCyclicLinks = function(startPoint) {
        let queue = [startPoint];
        let seenParents = new Set();
        seenParents.add(startPoint);
        while (queue.length) {
            let currentID = queue.shift();
            let parentList = components.childToParentsMap.get(currentID);
            if (parentList) {
                for (let parentID of parentList) {
                    if (parentID === startPoint && components.parentToChildrenMap.get(parentID).length > 1) {
                        components.childToParentsMap.get(currentID).splice(parentList.indexOf(parentID), 1);
                        if (components.childToParentsMap.get(currentID).length === 0) {
                            components.childToParentsMap.delete(currentID);
                        }
                        components.parentToChildrenMap.get(parentID).splice(parentList.indexOf(currentID), 1);
                        if (components.parentToChildrenMap.get(parentID).length === 0) {
                            components.parentToChildrenMap.delete(parentID);
                        }
                        if (components.cyclicLinks.get(parentID) === undefined) {
                            components.cyclicLinks.set(parentID, [currentID]);
                        } else {
                            components.cyclicLinks.get(parentID).push(currentID);
                        }
                    } else {
                        if (!seenParents.has(parentID)) {
                            seenParents.add(parentID);
                            queue.push(parentID);
                        }
                    }
                }
            }
        }
    };

    for (let parentID of components.childToParentsMap.keys()) {
        breakCyclicLinks(parentID);
    }
    return components;
};
/*
 * The Groups object takes a Components object and organises it into Group instances which it stores. It also houses methods for interacting with Group instances.
 */

const Groups = function(components, mainRoot) { 

    let groups = this;
    groups.groupsMap = new Map(); 
    groups.componentToGroupMap = new Map(); 

    
    groups.getComponentGroup = function(componentID) {
        return groups.groupsMap.get(groups.componentToGroupMap.get(componentID));
    };

    const Group = function(groupID, rootComponentID, groupIndex) {
        let group = this;
        group.id = groupID;
        group.index = groupIndex;
        group.inputs = [];
        group.outputs = [rootComponentID];
        group.tiles = new Tiles();
        group.layers = new Map();
        group.layerOffset = 0;
        group.components = new Set();

        group.addComponent = function(component) {
            group.components.add(component.id);
        };
        group.removeComponent = function(componentID) {
            group.components.delete(componentID);
        };
        group.size = function() {
            return group.components.size;
        };

        
        
        
        group.layer = function() {
            
            for (let output of group.outputs) {

                let startingLayer = 0;
                if (components.childToParentsMap.get(output)) {
                    for (let childID of components.childToParentsMap.get(output)) { 
                        startingLayer = Math.max(startingLayer, components.componentsMap.get(childID).layer - 1);
                    }
                }

                
                let queue = [];
                queue.push({componentID: output, layer: startingLayer});
                let seen = new Set();

                
                
                while (queue.length) {
                    let next = queue.shift();
                    let component = components.componentsMap.get(next.componentID);
                    if (next.layer > component.layer) {
                        if (component.layer > -1) { 
                            group.layers.get(component.layer).delete(component.id); 

                        }
                        component.layer = next.layer;
                        if (group.layers.has(component.layer)) {
                            let currentLayer = group.layers.get(component.layer);
                            currentLayer.add(next.componentID);
                            group.layers.set(component.layer, currentLayer);
                        } else {
                            group.layers.set(component.layer, new Set().add(next.componentID));
                        }
                    }

                    let layer = component.layer + 1;
                    let parents = components.childToParentsMap.get(next.componentID);
                    if (parents) {
                        for (let i = 0; i < parents.length; i++) {
                            if (group.components.has(parents[i])) {
                                seen.add(parents[i]);
                                queue.push({componentID: parents[i], layer: layer});
                            }
                        }
                    }
                }
            }
        };

        
        group.createTiles = function() {
            let layerIndex, layer, layerSize, lastLayer, lastLayerConnections, x, componentTile, y;
            layerIndex = group.layers.size - 1;
            let positionIndex = 0;
            let yStart = 1;
            while (group.layers.has(layerIndex)) {
                layer = group.layers.get(layerIndex);
                layerSize = layer.size;
                lastLayer = group.layers.get(layerIndex + 1);
                if (lastLayer) {
                    lastLayerConnections = lastLayer.size;
                    for (let output of group.outputs) {
                        if (lastLayer.has(output)) {
                            lastLayerConnections--;
                        }
                    }
                    layerSize = Math.max(lastLayerConnections, layer.size);
                }
                x = 1;
                yStart += Math.max(layerSize, 2);
                y = yStart;
                for (let componentID of layer) {
                    componentTile = group.tiles.createTile(x, y);
                    componentTile.groupID = group.id;
                    componentTile.componentID = componentID;
                    let neighbours = group.tiles.getNeighbours(componentTile);
                    for (let tile of neighbours) {
                        tile = tile.groupID = group.id;
                    }
                    x += 2;
                    y++;
                }
                positionIndex++;
                layerIndex--;
            }
        };
    };


    
    const createGroupFromRoot = function(rootComponentID, groupID, groupIndex) {
        let cameFrom = new Map();
        let group = new Group(groupID, rootComponentID, groupIndex);

        
        let componentIDqueue = [rootComponentID];
        let component;

        
        
        while (componentIDqueue.length) {
            let currentID = componentIDqueue.shift();
            let lastID = cameFrom.get(currentID);
            if (!groups.componentToGroupMap.has(currentID)) {
                groups.componentToGroupMap.set(currentID, groupID);
                component = components.componentsMap.get(currentID);
                group.addComponent(component);
            } else {
                if (groups.componentToGroupMap.get(currentID) !== groupID && groups.componentToGroupMap.get(lastID) === groupID) {
                    group.inputs.push(lastID);
                }
            }
            let parents = components.childToParentsMap.get(currentID);
            if (parents) {
                for (let parentID of parents) {
                    if (cameFrom.get(parentID) === undefined) {
                        cameFrom.set(parentID, currentID);
                        componentIDqueue.push(parentID);
                    }
                }
            } else {
                currentID = undefined;
            }
        }
        groups.groupsMap.set(groupID, group);
    };

    let groupIDNumber = 0;
    
    if (mainRoot) {
        createGroupFromRoot(mainRoot, 'group_' + groupIDNumber, groupIDNumber);
        components.roots.delete(mainRoot);
        groups.mainRoot = mainRoot;
    }
    for (let root of components.roots) {
        groupIDNumber++;
        createGroupFromRoot(root, 'group_' + groupIDNumber, groupIDNumber);
    }

    
    for (let [groupID, group] of groups.groupsMap) {
        if (group.components.size === 1) {
            let componentID = group.components.values().next().value;
            if (components.childToParentsMap.get(componentID) !== undefined && components.childToParentsMap.get(componentID).length > 0) {
                let parentID = components.childToParentsMap.get(componentID)[0];
                let newGroupID = groups.componentToGroupMap.get(parentID);
                let newGroup = groups.groupsMap.get(newGroupID);
                newGroup.components.add(componentID);
                newGroup.outputs.push(componentID);
                groups.groupsMap.set(newGroupID, newGroup);
                groups.componentToGroupMap.set(componentID, newGroupID);
                groups.groupsMap.delete(groupID);
            }
        }
    }

    
    for (let group of groups.groupsMap.values()) {
        group.layer();
        group.createTiles();
    }
};

const Directions = function() { 
    let directions = this;

    let xToYMap = new Map();
    let idMap = new Map();

    const Direction = function(name, index) {
        let direction = this;
        direction.name = name;
        direction.index = index;
    };

    const add = function(x, y, name, directionIndex) {
        let newDirection = new Direction(name, directionIndex);
        idMap.set(directionIndex, newDirection);
        if (xToYMap.has(x)) {
            xToYMap.get(x).set(y, newDirection);
        } else {
            xToYMap.set(x, new Map().set(y, newDirection));
        }
    };

    directions.get = function(id) {
        return idMap.get(id);
    };

    directions.getDirectionBetweenAdjacentCoordinates = function(x1, y1, x2, y2) {
        if (x1 - x2 > 1 || x1 - x2 < -1 || y1 - y2 > 1 || y1 - y2 < -1) {
            throw new Error('Could not calculate direction: coordinates are not adjacent');
        }
        return xToYMap.get(x1 - x2).get(y1 - y2);
    };

    add(0, -1, 'up', 0);
    add(1, 0, 'up-right', 1);
    add(1, 1, 'down-right', 2);
    add(0, 1, 'down', 3);
    add(-1, 0, 'down-left', 4);
    add(-1, -1, 'up-left', 5);

};

const Tiles = function(tileMap) { 
    let tiles = this;
    tiles.minX = null;
    tiles.maxX = null;
    tiles.minY = null;
    tiles.maxY = null;

    if (tileMap) {
        for (let tile of tileMap.values()) {
            if (this.minX === null || tile.x < this.minX) {
                this.minX = tile.x;
            }
            if (this.maxX === null || tile.x > this.maxX) {
                this.maxX = tile.x;
            }
            if (this.minY === null || tile.y < this.minY) {
                this.minY = tile.y;
            }
            if (this.maxY === null || tile.y > this.maxY) {
                this.maxY = tile.y;
            }
        }
        tiles.tileMap = tileMap;
    } else {
        tiles.tileMap = new Map();
    }

    tiles.getWidth = () => {
        return tiles.maxX + 1 - tiles.minX;
    };

    tiles.getHeight = () => {
        return tiles.maxY + 1 - tiles.minY;
    };

    function hexToCube(q, r) {
        return {
            x: q,
            z: r,
            y: -q-r
        };
    }

    function hexagonDistance(x1, y1, x2, y2) {
        let position1 = hexToCube(x1, y1);
        let position2 = hexToCube(x2, y2);
        return (Math.abs(position1.x - position2.x) + Math.abs(position1.y - position2.y) + Math.abs(position1.z - position2.z)) / 2;
    }

    const Tile = function(id, x, y) {
        let tile = this;
        tile.id = id;
        tile.x = x;
        tile.y = y;
        tile.componentID = null;
        tile.groupID = null;
        tile.connectionNodes = [];

        if (tiles.minX === null || x < tiles.minX) {
            tiles.minX = x;
        }
        if (tiles.maxX === null || x > tiles.maxX) {
            tiles.maxX = x;
        }
        if (tiles.minY === null || y < tiles.minY) {
            tiles.minY = y;
        }
        if (tiles.maxY === null || y > tiles.maxY) {
            tiles.maxY = y;
        }

        let cost = 0;

        tile.cost = function(newCost) {
            if (newCost === undefined) {
                return cost;
            } else {
                cost = newCost;
            }
        };

        tile.distanceToCoords = function(targetX, targetY) {
            return hexagonDistance(tile.x, tile.y, targetX, targetY);
        };

        tile.offsetGridDifference = function(tileToCompare) {
            let cube1 = hexToCube(tile.x, tile.y);
            let cube1col = cube1.x;
            let cube1row = cube1.z + (cube1.x + (cube1.x&1))/2;
            let cube2 = hexToCube(tileToCompare.x, tileToCompare.y);
            let cube2col = cube2.x;
            let cube2row = cube2.z + (cube2.x + (cube2.x&1))/2;
            return {x: cube1col - cube2col, y: cube1row - cube2row};
        };
    };

    const coordinatesToID = function(x, y) {
        return x + '_' + y;
    };

    tiles.createTile = function(x, y) {
        let id = coordinatesToID(x, y);
        let tile = new Tile(id, x, y);
        tiles.tileMap.set(id, tile);
        return tile;
    };

    tiles.getTileAtCoords = function(x, y) {
        return tiles.tileMap.get(coordinatesToID(x, y));
    };

    tiles.getNeighbours = function(tile) {
        return [
            tiles.getTileAtCoords(tile.x + 1, tile.y + 1)  || tiles.createTile(tile.x + 1, tile.y + 1),
            tiles.getTileAtCoords(tile.x, tile.y + 1)  || tiles.createTile(tile.x, tile.y + 1),
            tiles.getTileAtCoords(tile.x - 1, tile.y)  || tiles.createTile(tile.x - 1, tile.y),
            tiles.getTileAtCoords(tile.x - 1, tile.y - 1)  || tiles.createTile(tile.x - 1, tile.y - 1),
            tiles.getTileAtCoords(tile.x, tile.y - 1)  || tiles.createTile(tile.x, tile.y - 1),
            tiles.getTileAtCoords(tile.x + 1, tile.y)  || tiles.createTile(tile.x + 1, tile.y)
        ];
    };

    tiles.getTileById = function(id) {
        return tiles.tileMap.get(id);
    };

    tiles.getOffset = function(x, y) {
        let newTileMap = new Map();
        for (let tile of tiles.tileMap.values()) {
            let newID = coordinatesToID(tile.x + x, tile.y + y);
            let newTile = new Tile(newID, tile.x + x, tile.y + y);
            newTile.componentID = tile.componentID;
            newTile.groupID = tile.groupID;
            newTileMap.set(newID, newTile);
        }
        return newTileMap;
    };

    tiles.detectCollision = function(tileMapToTest) {
        let smallerGroup = (tileMapToTest.size <= tileMap.size) ? tileMapToTest: tileMap;
        let largerGroup = (tileMapToTest.size <= tileMap.size) ? tileMap: tileMapToTest;
        for (let tileID of smallerGroup.keys()) {
            if (largerGroup.has(tileID)) {
                return true;
            }
        }
        return false;
    };

};

const Connections = function(mainTilesGrid, options) { 
    let connections = this;
    connections.connectionsMap = new Map();
    let directions = new Directions();

    const distanceFactor = +options.distanceFactor || 2;
    const lineCost = +options.lineCost || 1;

    const ConnectionIDGen = function*() {
        let index = 0;
        while (true) {
            yield index++;
        }
    }();

    const CONNECTION_NODE_TYPES = {
        START: 'start',
        END: 'end',
        LINE: 'line',
        EDGE: 'edge'
    };

    const ConnectionNode = function(connectionID, tile, direction1, direction2) {
        let connectionNode = this;

        connectionNode.connectionID = connectionID;
        connectionNode.tile = tile;
        connectionNode.direction1 = direction1;
        connectionNode.direction2 = direction2;

        if (direction1 === null) {
            connectionNode.type = CONNECTION_NODE_TYPES.START;
        } else if (direction2 === null) {
            connectionNode.type = CONNECTION_NODE_TYPES.END;
        } else if (direction1 === direction2) {
            connectionNode.type = CONNECTION_NODE_TYPES.LINE;
        } else {
            connectionNode.type = CONNECTION_NODE_TYPES.EDGE;
        }
        tile.connectionNodes.push(connectionNode);
    };

    const Connection = function(id, tile1, tile2, path, inputIndex) {
        let connection = this;
        let direction1, direction2;

        connection.id = id;
        connection.components = new Set();
        connection.components.add(tile1.componentID);
        connection.components.add(tile2.componentID);
        connection.start = tile1.componentID;
        connection.end = tile2.componentID;
        connection.inputIndex = inputIndex;
        connection.path = [];
        connection.tiles = new Set(path);

        for (let i = 0; i < path.length; i++) {
            if (path[i - 1] === undefined) {
                direction1 = null;
            } else {
                direction1 = directions.getDirectionBetweenAdjacentCoordinates(path[i - 1].x, path[i - 1].y, path[i].x, path[i].y);
            }
            if (path[i + 1] === undefined) {
                direction2 = null;
            } else {
                direction2 = directions.getDirectionBetweenAdjacentCoordinates(path[i].x, path[i].y, path[i + 1].x, path[i + 1].y);
            }
            connection.path.push(new ConnectionNode(id, path[i], direction1, direction2));
        }

    };

    const borderGatewayIDGen = function*() {
        let index = 0;
        while (true) {
            yield 'ID' + index++;
        }
    }();

    const Border = function(index) {
        let border = this;
        border.index = index;
        border.connectionStartToBorderGatewayMap = new Map();
    };

    const BorderGateway = function(type) {
        let borderGate = this;
        borderGate.id = borderGatewayIDGen.next().value;
        borderGate.connectionNodes = new Set();
        borderGate.type = type;
    };

    connections.borderGatewayMap = new Map();
    const inToOutDirectionConversion = [3, 4, 5, 0, 1, 2];

    const MultiConnectionNode = function(tile, solvedNeighbours) {
        let multiConnectionNode = this;
        multiConnectionNode.tile = tile;
        multiConnectionNode.borderGateways = new Map();

        let borders = new Map();

        for (let i = 0; i < 6; i++) {
            borders.set(i, new Border(i));
        }

        let borderGatewayToBorderIndexMap = new Map();

        for (let connectionNode of tile.connectionNodes) {
            if (connectionNode.direction1 !== null) {
                if (solvedNeighbours[connectionNode.direction1.index] === null) {
                    let startID = connections.connectionsMap.get(connectionNode.connectionID).start;
                    if (!borders.get(connectionNode.direction1.index).connectionStartToBorderGatewayMap.has(startID)) {
                        let borderGateway = new BorderGateway('in');
                        borderGateway.connectionNodes.add(connectionNode);
                        borderGatewayToBorderIndexMap.set(borderGateway.id, connectionNode.direction1.index);
                        borders.get(connectionNode.direction1.index).connectionStartToBorderGatewayMap.set(startID, borderGateway.id);
                        multiConnectionNode.borderGateways.set(borderGateway.id, borderGateway);
                    } else {
                        let borderGatewayID = borders.get(connectionNode.direction1.index).connectionStartToBorderGatewayMap.get(startID);
                        multiConnectionNode.borderGateways.get(borderGatewayID).connectionNodes.add(connectionNode);
                    }
                }
            }
            if (connectionNode.direction2 !== null) {
                if (solvedNeighbours[inToOutDirectionConversion[connectionNode.direction2.index]] === null) {
                    let startID = connections.connectionsMap.get(connectionNode.connectionID).start;
                    if (!borders.get(inToOutDirectionConversion[connectionNode.direction2.index]).connectionStartToBorderGatewayMap.has(startID)) {
                        let borderGateway = new BorderGateway('out');
                        borderGateway.connectionNodes.add(connectionNode);
                        borderGatewayToBorderIndexMap.set(borderGateway.id, inToOutDirectionConversion[connectionNode.direction2.index]);
                        borders.get(inToOutDirectionConversion[connectionNode.direction2.index]).connectionStartToBorderGatewayMap.set(startID, borderGateway.id);
                        multiConnectionNode.borderGateways.set(borderGateway.id, borderGateway);
                    } else {
                        let borderGatewayID = borders.get(inToOutDirectionConversion[connectionNode.direction2.index]).connectionStartToBorderGatewayMap.get(startID);
                        multiConnectionNode.borderGateways.get(borderGatewayID).connectionNodes.add(connectionNode);
                    }
                }
            }
        }

        let borderIndex = 0;
        const borderFilter = function(x) {
            return x[1] === borderIndex;
        };

        let borderToSortedBorderGatewayArray = new Map();

        for (let i = 0; i < 6; i++) {
            if (solvedNeighbours[i] !== null) {
                let solvedBorderArray = solvedNeighbours[i].borderToSortedBorderGatewayArray.get(inToOutDirectionConversion[i]);
                let newBorderArray = [];
                for (let v = solvedBorderArray.length - 1; v >= 0; v--) {
                    let oldBorderGateway = solvedNeighbours[i].borderGateways.get(solvedBorderArray[v]);
                    let clonedBorderGateway = new BorderGateway(oldBorderGateway.type === 'in'? 'out': 'in');
                    for (let connectionNode of solvedNeighbours[i].borderGateways.get(solvedBorderArray[v]).connectionNodes) {
                        clonedBorderGateway.connectionNodes.add(connectionNode);
                    }
                    multiConnectionNode.borderGateways.set(clonedBorderGateway.id, clonedBorderGateway);
                    newBorderArray.push(clonedBorderGateway.id);
                }
                borderToSortedBorderGatewayArray.set(borderIndex, newBorderArray);
            } else {
                let leftBorderGatewayArray = [];
                let centreBorderGatwayArray = [];
                let rightBorderGatewayArray = [];
                let borderGatewayArray = [...borderGatewayToBorderIndexMap].filter(borderFilter);
                let furthestAngle = 0;
                for (let j = 0; j < borderGatewayArray.length; j++) {
                    let borderGatewayID = borderGatewayArray[j][0];
                    let entryBorder = borderGatewayArray[j][1];
                    for (let connectionNode of multiConnectionNode.borderGateways.get(borderGatewayID).connectionNodes) {
                        let connection = connections.connectionsMap.get(connectionNode.connectionID);
                        let connectionNodePoint = connection.path.indexOf(connectionNode);
                        let nearestAngle = 0;
                        for (let k = connectionNodePoint; k < connection.path.length; k++) {
                            if (connection.path[k].direction1 !== connection.path[k].direction2) {
                                if (connection.path[k].direction2 === null) {
                                    centreBorderGatwayArray.push([borderGatewayID, 0, entryBorder]);
                                } else {
                                    furthestAngle = Math.max(furthestAngle, nearestAngle);
                                    let d1 = connection.path[k].direction1 !== null ? connection.path[k].direction1.index : 3;
                                    let d2 = connection.path[k].direction2 !== null ? connection.path[k].direction2.index : 0;
                                    let angleIndex = d2 - d1 < 0 ? d1 - d2 + 6 : d1 - d2;
                                    if (angleIndex < 3) {
                                        if (multiConnectionNode.borderGateways.get(borderGatewayID).type === 'in') {
                                            leftBorderGatewayArray.push([borderGatewayID, nearestAngle, entryBorder]);
                                        } else {
                                            rightBorderGatewayArray.push([borderGatewayID, nearestAngle, inToOutDirectionConversion[entryBorder]]);
                                        }
                                    } else {
                                        if (multiConnectionNode.borderGateways.get(borderGatewayID).type === 'in') {
                                            rightBorderGatewayArray.push([borderGatewayID, nearestAngle, entryBorder]);
                                        } else {
                                            leftBorderGatewayArray.push([borderGatewayID, nearestAngle, inToOutDirectionConversion[entryBorder]]);
                                        }
                                    }
                                }
                                break;
                            }
                            nearestAngle++;
                        }
                        break;
                    }
                }

                const sortBorderGateways = function(a, b) {
                    if (a[1] === b[1]) {
                        return b[2] - a[2];
                    }
                    return a[1] - b[1];
                };

                leftBorderGatewayArray.sort(sortBorderGateways);
                centreBorderGatwayArray.sort(sortBorderGateways);
                rightBorderGatewayArray.sort(sortBorderGateways);

                let sortedBorderGateways = [];
                for (let data of leftBorderGatewayArray) {
                    sortedBorderGateways.push(data[0]);
                }
                for (let data of centreBorderGatwayArray) {
                    sortedBorderGateways.push(data[0]);
                }
                for (let data of rightBorderGatewayArray) {
                    sortedBorderGateways.push(data[0]);
                }

                borderToSortedBorderGatewayArray.set(borderIndex, sortedBorderGateways);
            }
            borderIndex++;
        }

        multiConnectionNode.borderToSortedBorderGatewayArray = borderToSortedBorderGatewayArray;
    };

    const CostAdjustments = function() {
        let costAdjustments = this;
        let adjustments = new Map();

        let getAdjustment = function(tileID) {
            return adjustments.has(tileID) ? adjustments.get(tileID): 0;
        };

        costAdjustments.update = function(tileID, costAdjustment) {
            if (adjustments.has(tileID)) {
                adjustments.set(tileID, getAdjustment(tileID) + costAdjustment);
            } else {
                adjustments.set(tileID, costAdjustment);
            }
        };

        costAdjustments.get = function(tileID) {
            return getAdjustment(tileID);
        };

    };

    connections.connectTiles = function(tile1, tile2, inputIndex) {
        let temporaryCostAdjustments = new CostAdjustments();
        let tile1Neighbours = mainTilesGrid.getNeighbours(tile1);
        let tile2Neighbours = mainTilesGrid.getNeighbours(tile2);
        let sourceTile = tile1Neighbours[1];
        let targetTile = tile2Neighbours[4];
        let frontier = new PriorityQueue();
        frontier.push(sourceTile.id, 0);
        let cameFrom = {};
        cameFrom[sourceTile.id] = 'none';
        let costAtPosition = {};
        costAtPosition[sourceTile.id] = 0;

        let groupSet = new Set([tile1.groupID, tile2.groupID]);
        let existingConnectionNodes = new Set();
        for (let connection of connections.connectionsMap.values()) {
            if (connection.end === tile2.componentID) {
                for (let connectionNode of connection.path) {
                    existingConnectionNodes.add(connectionNode);
                }
            }
        }

        let currentTile, lastTile, lastDirection;
        while (frontier.size()) {
            let node = frontier.next();
            let currentID = node.data;
            currentTile = mainTilesGrid.getTileById(currentID);
            if (currentTile === targetTile) {
                break;
            }
            let neighbours = mainTilesGrid.getNeighbours(currentTile);
            for (let neighbourTile of neighbours) {
                let nextID = neighbourTile.id;
                let projectedDirection = directions.getDirectionBetweenAdjacentCoordinates(currentTile.x, currentTile.y, neighbourTile.x, neighbourTile.y);
                if (cameFrom[currentID] !== 'none') {
                    lastTile =  mainTilesGrid.getTileById(cameFrom[currentID]);
                    lastDirection = directions.getDirectionBetweenAdjacentCoordinates(lastTile.x, lastTile.y, currentTile.x, currentTile.y);
                }
                if (!cameFrom[nextID] && (neighbourTile.componentID === null || (neighbourTile.x === targetTile.x && neighbourTile.y === targetTile.y))) {
                    if (groupSet.has(neighbourTile.groupID) || neighbourTile.groupID === null) {
                        let adjustment = 0;
                        if (cameFrom[currentID] !== 'none') {
                            if (lastDirection === projectedDirection) {
                                adjustment -= 1;
                            }
                        }

                        for (let connectionNode of neighbourTile.connectionNodes) {
                            if (connections.connectionsMap.get(connectionNode.connectionID).start === tile1.componentID || connections.connectionsMap.get(connectionNode.connectionID).end === tile2.componentID) {
                                adjustment -= 2; 
                            }
                            if (connections.connectionsMap.get(connectionNode.connectionID).end !== tile2.componentID) {
                                adjustment += 1; 
                            }
                        }

                        temporaryCostAdjustments.update(neighbourTile.id, adjustment);
                        let distance = neighbourTile.distanceToCoords(targetTile.x, targetTile.y);
                        if (sourceTile.groupID !== targetTile.groupID) {
                            for (let connectionNode of existingConnectionNodes) {
                                distance = Math.min(distance, neighbourTile.distanceToCoords(connectionNode.tile.x, connectionNode.tile.y));
                            }
                        } else {
                            if (existingConnectionNodes.size > 0) {
                                distance = NaN;
                            }
                        }

                        let costAdjustment = temporaryCostAdjustments.get(neighbourTile.id);
                        let newCost = costAtPosition[currentID] + neighbourTile.cost() + (distance * distanceFactor) + costAdjustment;
                        frontier.push(nextID, newCost);
                        cameFrom[nextID] = currentID;
                        costAtPosition[nextID] = newCost;
                    }
                }
            }
        }

        let tilePath = [tile2, currentTile];
        currentTile.cost(currentTile.cost() + lineCost);
        let currentID = currentTile.id;
        while (cameFrom[currentID] !== 'none') {
            let parentID = cameFrom[currentID];
            let tile = mainTilesGrid.getTileById(parentID);
            tile.cost(tile.cost() + lineCost);
            tilePath.push(tile);
            currentID = parentID;
        }
        tilePath.push(tile1);

        if (tile1.groupID === tile2.groupID) {
            for (let tile of tilePath) {
                tile.groupID = tile1.groupID;
            }
        }
        let connectionID = ConnectionIDGen.next().value;
        connections.connectionsMap.set(connectionID, new Connection(connectionID, tile1, tile2, tilePath, inputIndex));
        connections.tileIDToMultiConnectionNode = new Map();
        return connectionID;
    };

    connections.tileIDToMultiConnectionNode = new Map();

    connections.createMultiConnectionNode = function(tile, solvedNeighbours) {
        let multiConnectionNode = new MultiConnectionNode(tile, solvedNeighbours);
        connections.tileIDToMultiConnectionNode.set(tile.id, multiConnectionNode);
    };

};

const Layout = function(components, linkedComponentsToInputIndex, options) { 
    let layout = this;
    layout.components = components;
    layout.groups = new Groups(layout.components, options.mainRoot);

    const checkForCollisions = function(tileGrid, tileMapToTest) {
        for (let id of tileMapToTest.keys()) {
            if (tileGrid.has(id)) {
                return true;
            }
        }
        return false;
    };

    const findTileWithComponentID = function(tileGrid, componentID) {
        for (let tile of tileGrid.values()) {
            if (tile.componentID === componentID) {
                return tile;
            }
        }
        return false;
    };


    const joinGroups = function() {
        let tileGrid = null;
        let childToParentMap = components.childToParentsMap;
        let collisionFound = true;

        let maxX = null, maxY = null, offsetTileMap;

        let groupQueue = new PriorityQueue();
        for (let group of layout.groups.groupsMap.values()) {
            groupQueue.push(group, group.index);
        }

        while (groupQueue.size()) {
            let node = groupQueue.next();
            let group = node.data;
            if (tileGrid === null) {
                tileGrid = group.tiles.tileMap;
                maxX = group.tiles.maxX;
                maxY = group.tiles.maxY;
            } else {
                if (group.inputs.length > 0) {
                    let targetParentIDQueue = new PriorityQueue();
                    for (let inputID of group.inputs) {
                        for (let parentID of childToParentMap.get(inputID)) {
                            if (!group.components.has(parentID)) {
                                targetParentIDQueue.push(parentID, components.componentsMap.get(parentID).layer - layout.groups.groupsMap.get(layout.groups.componentToGroupMap.get(parentID)).layerOffset);
                            }
                        }
                    }
                    node = targetParentIDQueue.next();
                    group.layerOffset = node.cost;

                    let targetParentTile = findTileWithComponentID(tileGrid, node.data);
                    let x = targetParentTile.x + 1;
                    let y = targetParentTile.y + 1;
                    collisionFound = true;
                    while (collisionFound) {
                        offsetTileMap = group.tiles.getOffset(x, y);
                        if (checkForCollisions(tileGrid, offsetTileMap)) {
                            x += 6;
                            y += 3;
                        } else {
                            collisionFound = false;
                        }
                    }
                    tileGrid = new Map([...tileGrid, ...offsetTileMap]);
                }
                else {
                    let width = group.tiles.getWidth();
                    offsetTileMap = group.tiles.getOffset(maxX + 5, Math.floor((maxX + 10)/2));
                    maxX += width + 5;
                    tileGrid = new Map([...tileGrid, ...offsetTileMap]);
                }
            }
        }
        layout.tileGrid = tileGrid;
        layout.maxX = maxX;
        layout.maxY = maxY;
    };
    joinGroups();
    layout.tiles = new Tiles(layout.tileGrid);
    layout.connections = new Connections(layout.tiles, options);

    const connectComponents = function(componentID1, componentID2) {
        let tileMap = layout.tiles.tileMap;
        let tile1 = findTileWithComponentID(tileMap, componentID1);
        let tile2 = findTileWithComponentID(tileMap, componentID2);
        if (tile1 && tile2) {
            let inputIndex = linkedComponentsToInputIndex.get(componentID1 + '_' + componentID2);
            return layout.connections.connectTiles(tile1, tile2, inputIndex);
        }
    };


    for (let [childID, parentsList] of [...layout.components.childToParentsMap.entries()].reverse()) {
        if (parentsList && parentsList !== 'none') {
            for (let parentID of parentsList) {
                connectComponents(parentID, childID);
            }
        }
    }

    layout.cyclicConnections = new Set();
    for (let [parentID, childrenList] of layout.components.cyclicLinks) {
        for (let childID of childrenList) {
            let connectionID = connectComponents(parentID, childID);
            layout.cyclicConnections.add(connectionID);
        }
    }


    const getSolvedNeighbours = function(tile) {
        return [
            layout.connections.tileIDToMultiConnectionNode.get(tile.x + '_' + (tile.y - 1)) || null,
            layout.connections.tileIDToMultiConnectionNode.get((tile.x + 1) + '_' + tile.y) || null,
            layout.connections.tileIDToMultiConnectionNode.get((tile.x + 1) + '_' + (tile.y + 1)) || null,
            layout.connections.tileIDToMultiConnectionNode.get(tile.x + '_' + (tile.y + 1)) || null,
            layout.connections.tileIDToMultiConnectionNode.get((tile.x - 1) + '_' + tile.y) || null,
            layout.connections.tileIDToMultiConnectionNode.get((tile.x - 1) + '_' + (tile.y - 1)) || null
        ];
    };

    for (let connection of layout.connections.connectionsMap.values()) {
        for (let connectionNode of connection.path) {
            let tile = connectionNode.tile;
            if (tile.componentID === null && tile.connectionNodes.length > 1) {
                let solvedNeighbours = getSolvedNeighbours(tile);
                layout.connections.createMultiConnectionNode(tile, solvedNeighbours);
            }
        }
    }

    for (let tile of layout.tiles.tileMap.values()) {
        if (tile.componentID !== null) {
            let solvedNeighbours = getSolvedNeighbours(tile);
            layout.connections.createMultiConnectionNode(tile, solvedNeighbours);
        }
    }

};

const CreateView = function(layout, options) { 
    let view = this;
    options = options || {};
    view.width = options.width || 2000;
    view.height = options.height || 1000;
    view.cellRadius = options.cellRadius || 30;

    let groupHues = new Map();
    let hue = 0;
    let svg = {
        polygon: function(points, attributes) {
            return {points: points, attributes: attributes};
        },
        polyline: function(points, attributes) {
            return {points: points, attributes: attributes};
        },
        circle: function(attributes) {
            return {attributes: attributes};
        },
        text: function(text, attributes) {
            return {text: text, attributes: attributes};
        }
    };

    let tileSVGData = [];
    let componentSVGData = [];
    let connectionSVGData = [];
    let multiConnectionTileSVGData = [];
    let borderGatewayConnectionSVGData = [];

    const hueStep = 255/layout.groups.groupsMap.size;
    for (let groupID of layout.groups.groupsMap.keys()) {
        groupHues.set(groupID, hue);
        hue += hueStep;
    }

    function convertCoordinatesToScreen(x, y) {
        let squareCellHeight = Math.sqrt(Math.pow(view.cellRadius, 2) - Math.pow(view.cellRadius/2, 2)) * 2;
        let squareCellWidth = (view.cellRadius * 2) - (view.cellRadius/2);
        let offset = squareCellHeight/2;
        if (x % 2 !== 0) {
            offset = squareCellHeight;
        }
        return {
            x: (x * squareCellWidth) + 100,
            y: ((y * squareCellHeight) - (Math.ceil(x/2) * squareCellHeight) + offset) + 100
        };
    }

    function rotatePoint(pointX, pointY, originX, originY, angle) {
        angle = angle * Math.PI / 180.0;
        return {
            x: Math.cos(angle) * (pointX-originX) - Math.sin(angle) * (pointY-originY) + originX,
            y: Math.sin(angle) * (pointX-originX) + Math.cos(angle) * (pointY-originY) + originY
        };
    }

    const hsl = (h, s ,l) => {
        h = h/360;
        s = s/100;
        l = l/100;
        var r, g, b;
        if (s === 0) {
            r = g = b = l;
        } else {
            var hue2rgb = function hue2rgb(p, q, t){
                if (t < 0) {
                    t += 1;
                }
                if (t > 1) {
                    t -= 1;
                }
                if(t < 1/6) {
                    return p + (q - p) * 6 * t;
                }
                if (t < 1/2) {
                    return q;
                }
                if (t < 2/3) {
                    return p + (q - p) * (2 / 3 - t) * 6;
                }
                return p;
            };

            var q = l < 0.5 ? l * (1 + s) : l + s - l * s;
            var p = 2 * l - q;
            r = hue2rgb(p, q, h + 1/3);
            g = hue2rgb(p, q, h);
            b = hue2rgb(p, q, h - 1/3);
        }
        return `#${Math.round(r * 255).toString(16)}${Math.round(g * 255).toString(16)}${Math.round(b * 255).toString(16)}`;
    };

    function createTileSVG(tile) {
        let x = tile.x;
        let y = tile.y;
        let screenCoordinates = convertCoordinatesToScreen(x, y);
        let screenX = screenCoordinates.x;
        let screenY = screenCoordinates.y;
        let startPoint = {x: screenX + view.cellRadius, y: screenY};
        let hexPoints = [[startPoint.x, startPoint.y]];
        for (let i = 1; i < 6; i++) {
            let coords = rotatePoint(startPoint.x, startPoint.y, screenX, screenY, i * 60);
            if (coords.x < 0 || coords.y < 0) {
                return false;
            }
            hexPoints.push([coords.x, coords.y]);
        }
        hexPoints.push([startPoint.x, startPoint.y]);
        let fill = tile.groupID === null ? 'white': hsl(groupHues.get(tile.groupID), 50, 90);
        let stroke = hsl(0, 0, 85);
        let hexCell = svg.polygon(hexPoints, {class: 'tile', 'data-tile-id': tile.id, 'stroke-width': 1, stroke: stroke, fill: fill});
        if (tile.componentID !== null) {
            drawComponent(tile.componentID, tile);
        }
        return hexCell;
    }

    function drawComponent(componentID, tile) {
        let component = layout.components.componentsMap.get(componentID);
        let screenCoordinates = convertCoordinatesToScreen(tile.x, tile.y);
        let screenX = screenCoordinates.x;
        let screenY = screenCoordinates.y;
        let componentPoints = [
            [screenX + view.cellRadius/2, screenY - view.cellRadius/2],
            [screenX + view.cellRadius/2, screenY + view.cellRadius/2],
            [screenX - view.cellRadius/2, screenY + view.cellRadius/2],
            [screenX - view.cellRadius/2, screenY - view.cellRadius/2],
            [screenX + view.cellRadius/2, screenY - view.cellRadius/2]
        ];

        let hue = groupHues.get(tile.groupID);
        let fill = hsl(hue, 40, 50);

        if (component.type === 'switch') {
            componentPoints = [
                [screenX + view.cellRadius/2, screenY - view.cellRadius/3],
                [screenX + view.cellRadius/3, screenY + view.cellRadius/3],
                [screenX - view.cellRadius/3, screenY + view.cellRadius/3],
                [screenX - view.cellRadius/2, screenY - view.cellRadius/3],
                [screenX + view.cellRadius/2, screenY - view.cellRadius/3]
            ];
            fill = hsl(hue, 40, 65);
        }

        if (component.type === 'combiner') {
            componentPoints = [
                [screenX + view.cellRadius/2, screenY - view.cellRadius/4],
                [screenX, screenY + view.cellRadius/2],
                [screenX, screenY + view.cellRadius/2],
                [screenX - view.cellRadius/2, screenY - view.cellRadius/4],
                [screenX + view.cellRadius/2, screenY - view.cellRadius/4]
            ];
            fill = hsl(hue, 40, 35);
        }

        let stroke = hsl(hue, 90, 50);
        let componentBlock = svg.polygon(componentPoints, {class: 'component ', 'data-component-id': componentID, 'data-component-name': component.name, 'data-component-type': component.type, stroke: stroke, 'stroke-width': 3, 'stroke-opacity': 0.4, fill: fill, 'stroke-linecap': 'round', 'pointer-events': 'visiblePainted'});

        let inputLabel = svg.circle({class: 'component-input-label', 'data-component-id': componentID, fill: 'green', r: '9', cx: componentPoints[0][0] + 6, cy: componentPoints[0][1] - 9, opacity: 0});
        let inputLabelText = svg.text('', {class: 'component-input-label-text', 'data-component-id': componentID, 'text-anchor': 'middle', x: componentPoints[0][0] + 6, y: componentPoints[0][1] - 5, stroke: 'white', fill: 'white', opacity: 0});
        let text = svg.text(component.name, {class: 'component-name', x: screenX, y: screenY, 'font-size': 8, 'text-anchor': 'middle', 'pointer-events': 'none', fill: '#424242'});

        componentSVGData.push({text: text, block: componentBlock, inputLabel: inputLabel, inputLabelText: inputLabelText});
    }

    const drawConnection = function(connection) {
        let screenCoordinatePath = [];
        let convertedCoords;
        let i, tile;
        for (i = 0; i < connection.path.length; i++) {
            tile = connection.path[i].tile;
            convertedCoords = convertCoordinatesToScreen(tile.x, tile.y);
            screenCoordinatePath.push([convertedCoords.x, convertedCoords.y]);
        }
        let groupHue = groupHues.get(tile.groupID);
        let lineColour = layout.cyclicConnections.has(connection.id) ? 'black': hsl(groupHue, 70, 50);
        let movingHighlight = svg.polyline(screenCoordinatePath, {'class': 'moving-connection-highlight connection-highlight', 'stroke-dasharray': '10, 15', 'data-component-id': connection.end, 'data-connection-id': connection.id, 'stroke-width': 4, stroke: 'white', 'stroke-linecap': 'round', opacity: 0, 'fill': 'none'});
        let lineHighlight = svg.polyline(screenCoordinatePath, {'class': 'connection-highlight', 'data-component-id': connection.end, 'data-connection-id': connection.id, 'stroke-width': 4, stroke: hsl(groupHue, 100, 50), 'stroke-linecap': 'round', opacity: 0, 'fill': 'none'});
        let line = svg.polyline(screenCoordinatePath, {'class': 'connection', 'data-input-index': connection.inputIndex, 'data-connection-id': connection.id, 'stroke-width': 2, 'stroke': lineColour, opacity: 1, 'fill': 'none', 'stroke-linecap': 'round', 'pointer-events': 'none'});
        let lineHover = svg.polyline(screenCoordinatePath, {'class': 'connection-hover', 'data-input-index': connection.inputIndex, 'data-component-id': connection.end, 'data-connection-id': connection.id, 'cursor': 'pointer', 'stroke-width': 15, 'stroke': 'black', opacity: 0, 'fill': 'none'});

        connectionSVGData.push({line: line, highlight: lineHighlight, hover: lineHover, movingHighlight: movingHighlight});
    };

    const rotatePointToBorderIndex = function(x, y, cx, cy, borderIndex) {
        return rotatePoint(x, y, cx, cy, borderIndex * 60);
    };

    const drawBorderGatewayConnection = function(borderGateway1Coordinates, borderGateway2Coordinates, connectionID, groupHue) {
        let connection = layout.connections.connectionsMap.get(connectionID);
        let lineColour = layout.cyclicConnections.has(connection.id) ? 'black': hsl(groupHue, 70, 50);

        let movingHighlight = svg.polyline([[borderGateway1Coordinates.x, borderGateway1Coordinates.y], [borderGateway2Coordinates.x, borderGateway2Coordinates.y]], {'class': 'moving-connection-highlight connection-highlight', 'stroke-dasharray': '10, 15', 'stroke-linecap': 'round', 'data-component-id': connection.end, 'data-connection-id': connectionID, 'stroke-width': 4, stroke: 'white', opacity: 0, 'fill': 'none'});
        let lineHighlight = svg.polyline([[borderGateway1Coordinates.x, borderGateway1Coordinates.y], [borderGateway2Coordinates.x, borderGateway2Coordinates.y]], {'class': 'connection-highlight', 'stroke-linecap': 'round', 'data-component-id': connection.end, 'data-connection-id': connectionID, 'stroke-width': 4, stroke: hsl(groupHue, 100, 50), opacity: 0, 'fill': 'none'});
        let line = svg.polyline([[borderGateway1Coordinates.x, borderGateway1Coordinates.y], [borderGateway2Coordinates.x, borderGateway2Coordinates.y]], {'class': 'connection', 'data-connection-id': connectionID, stroke: lineColour, fill: 'none', 'stroke-width': 2, 'stroke-linecap': 'round'});
        let lineHover = svg.polyline([[borderGateway1Coordinates.x, borderGateway1Coordinates.y], [borderGateway2Coordinates.x, borderGateway2Coordinates.y]], {'class': 'connection-hover', 'data-input-index': connection.inputIndex, 'data-component-id': connection.end, 'data-connection-id': connection.id, 'cursor': 'pointer', 'stroke-width': 15, 'stroke': 'black', opacity: 0, 'fill': 'none'});
        borderGatewayConnectionSVGData.push({line: line, highlight: lineHighlight, hover: lineHover, movingHighlight: movingHighlight});
    };

    const drawMultiConnection = function(multiConnectionNode) {
        let tileSVG = createTileSVG(multiConnectionNode.tile);
        multiConnectionTileSVGData.push(tileSVG);

        let screenTileCentre = convertCoordinatesToScreen(multiConnectionNode.tile.x, multiConnectionNode.tile.y);
        let cx = screenTileCentre.x;
        let cy = screenTileCentre.y;

        let borderGatewayIDToScreenCoordinates = new Map();

        for (let [borderIndex, borderGatewayArray] of multiConnectionNode.borderToSortedBorderGatewayArray) {
            let borderGatewaySpacing = view.cellRadius/(borderGatewayArray.length + 1);
            let currentPortPosition = borderGatewaySpacing;
            for (let borderGatewayID of borderGatewayArray) {
                let borderGatewayCoordinates = rotatePointToBorderIndex(cx - (view.cellRadius/2) + currentPortPosition, cy - Math.sqrt(Math.pow(view.cellRadius, 2) - Math.pow(view.cellRadius/2, 2)), cx, cy, borderIndex);
                borderGatewayIDToScreenCoordinates.set(borderGatewayID, borderGatewayCoordinates);
                currentPortPosition += borderGatewaySpacing;
            }
        }
        if (multiConnectionNode.tile.componentID !== null) {
            for (let [id, borderGateway] of multiConnectionNode.borderGateways) {
                for (let connectionNode of borderGateway.connectionNodes) {
                    let connectionID = connectionNode.connectionID;
                    let groupHue = groupHues.get(layout.groups.componentToGroupMap.get(layout.connections.connectionsMap.get(connectionNode.connectionID).start));
                    drawBorderGatewayConnection(borderGatewayIDToScreenCoordinates.get(id), {
                        x: borderGatewayIDToScreenCoordinates.get(id).x,
                        y: screenTileCentre.y
                    }, connectionID, groupHue);
                }
            }
        } else {
            for (let borderGateway of multiConnectionNode.borderGateways.values()) {
                if (borderGateway.type === 'in') {
                    for (let connectionNode of borderGateway.connectionNodes) {
                        let groupHue = groupHues.get(layout.groups.componentToGroupMap.get(layout.connections.connectionsMap.get(connectionNode.connectionID).start));
                        for (let borderGateway2 of multiConnectionNode.borderGateways.values()) {
                            for (let connectionNode2 of borderGateway2.connectionNodes) {
                                if (connectionNode.connectionID === connectionNode2.connectionID &&
                                    borderGateway.type !== borderGateway2.type
                                ) {
                                    drawBorderGatewayConnection(borderGatewayIDToScreenCoordinates.get(borderGateway.id), borderGatewayIDToScreenCoordinates.get(borderGateway2.id), connectionNode.connectionID, groupHue, borderGateway, borderGateway);
                                }
                            }
                        }
                    }
                }
            }
        }
    };


    for (let tile of layout.tileGrid.values()) {
        if (tile.groupID !== null) {
            let tileSVG = createTileSVG(tile);
            if (tileSVG) {
                tileSVGData.push(tileSVG);
            }
        }
    }

    for (let connection of layout.connections.connectionsMap.values()) {
        drawConnection(connection);
    }

    for (let multiConnectionNode of layout.connections.tileIDToMultiConnectionNode.values()) {
        drawMultiConnection(multiConnectionNode);
    }

    return {
        tileSVGData: tileSVGData,
        componentSVGData: componentSVGData,
        connectionSVGData: connectionSVGData,
        borderGatewayConnectionSVGData: borderGatewayConnectionSVGData,
        multiConnectionTileSVGData: multiConnectionTileSVGData
    };

};
self.addEventListener('message', function(e) {
    self.postMessage({type: 'status', message: 'Creating Components object...'});
    let components = new Components(e.data.components);
    self.postMessage({type: 'status', message: 'Done.'});
    self.postMessage({type: 'status', message: 'Making components object acyclic...'});
    let acyclicComponents = MakeAcyclic(components);
    self.postMessage({type: 'status', message: 'Done.'});
    self.postMessage({type: 'status', message: 'Creating Layout for Components object...'});
    let layout = new Layout(acyclicComponents, e.data.linkedComponentsToInputIndex, e.data.options);
    self.postMessage({type: 'status', message: 'Done.'});
    self.postMessage({type: 'status', message: 'Creating View for Layout object...'});
    self.postMessage({type: 'status', message: 'Done.'});
    let view = CreateView(layout);
    self.postMessage(view);
}, false);
