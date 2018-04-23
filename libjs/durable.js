exports = module.exports = durableEngine = function () {
    var d = require('./durableEngine');
    var r = require('bindings')('rulesjs.node');
    var ep = require('esprima');
    var ec = require('escodegen');
    var dh = d.host();
    var dc = d.closure();

    var omap = {
        '+': 'add',
        '==': 'eq',
        '===': 'eq',
        '!=': 'neq',
        '!==': 'neq',
        '||': 'or',
        '&&': 'and',
        '<': 'lt',
        '>': 'gt',
        '<=': 'lte',
        '>=': 'gte',
        '-': 'sub',
        '*': 'mul',
        '/': 'div',
        'u+': 'ex',
        'u~': 'nex',
    };

    var transformStartStatement = function (statement) {
        switch (statement.type) {
            case 'VariableDeclaration':
                statement.declarations.forEach(function (declaration, idx) {
                    transformStartStatement(declaration.init);
                });
                break;
            case 'BinaryExpression':
            case 'LogicalExpression':               
                transformStartStatement(statement.left);
                transformStartStatement(statement.right);
                break;
            case 'ExpressionStatement':
                transformStartStatement(statement.expression);
                break;
            case 'CallExpression':
                statement['arguments'].forEach(function (argument, idx) {
                    transformStartStatement(argument);
                });

                if (statement.callee.type === 'Identifier' && 
                    dh[statement.callee.name]) {
                    statement.callee = {
                        type: 'MemberExpression',
                        object: {
                            type: 'Identifier',
                            name: 'host'
                        },
                        property: {
                            type: 'Identifier',
                            name: statement.callee.name
                        }  
                    }; 
                }
                
                break;
            case 'AssignmentExpression':
                transformStartStatement(statement.left);
                transformStartStatement(statement.right);
                break;
            case 'UnaryExpression':
                transformStartStatement(statement.argument);
                break;
            case 'FunctionDeclaration':
            case 'FunctionExpression':
                transformStartStatement(statement.body);
                break;
            case 'BlockStatement':
                statement.body.forEach(function (statement) {
                    transformStartStatement(statement);
                });
                break;
            case 'ReturnStatement':
                transformStartStatement(statement.argument);
                break;
            case 'SwitchStatement':
               statement.cases.forEach(function(c, i){
                  transformStartStatement(c);
               });
                break;
            case 'SwitchCase':
               statement.consequent.forEach(function(c, i){
                   transformStartStatement(c);
               });
                break;
            case 'IfStatement':
                transformStartStatement(statement.test);
                transformStartStatement(statement.consequent);
                if (statement.alternate) {
                    transformStartStatement(statement.alternate);   
                }
                break;
            case 'ObjectExpression':
                statement.properties.forEach(function(prop, i) {
                    transformStartStatement(prop.key);
                    transformStartStatement(prop.value);
                });
            case 'MemberExpression':
            case 'UpdateExpression':
            case 'Literal':
            case 'Identifier':
                break;
        }
    }

    var transformStartStatements = function (block, cmap) {
        var statements;
        if (block.type !== 'BlockStatement') {
            statements = [ block ];
        } else {
            statements = block.body;
        }

        statements.forEach(function (statement, index) {
            transformStartStatement(statement, cmap);
        });

        return {
            type: 'FunctionExpression',
            params: [{type:'Identifier', name:'host'}],
            body: { type: 'BlockStatement', body: statements }
        };
    }

    var transformRunStatement = function (statement, cmap) {
        switch (statement.type) {
            case 'VariableDeclaration':
                statement.declarations.forEach(function (declaration, idx) {
                    transformRunStatement(declaration.init, cmap);
                });
                break;
            case 'BinaryExpression':
            case 'LogicalExpression':               
                transformRunStatement(statement.left, cmap);
                transformRunStatement(statement.right, cmap);
                break;
            case 'ExpressionStatement':
                transformRunStatement(statement.expression, cmap);
                break;
            case 'CallExpression':
                statement['arguments'].forEach(function (argument, idx) {
                    transformRunStatement(argument, cmap);
                });

                if (statement.callee.type !== 'Identifier' ||
                    !dc[statement.callee.name]) {
                    transformRunStatement(statement.callee, cmap);
                } else {
                    statement.callee = {
                        type: 'MemberExpression',
                        object: {
                            type: 'Identifier',
                            name: 'c'
                        },
                        property: {
                            type: 'Identifier',
                            name: statement.callee.name
                        }  
                    }; 
                }
                
                break;
            case 'AssignmentExpression':
                transformRunStatement(statement.left, cmap);
                transformRunStatement(statement.right, cmap);
                break;
            case 'UnaryExpression':
                transformRunStatement(statement.argument, cmap);
                break;
            case 'FunctionDeclaration':
            case 'FunctionExpression':
                transformRunStatement(statement.body, cmap);
                break;
            case 'BlockStatement':
                statement.body.forEach(function (statement) {
                    transformRunStatement(statement, cmap);
                });
                break;
            case 'ReturnStatement':
                transformRunStatement(statement.argument, cmap);
                break;
            case 'MemberExpression':
                if (statement.object.type === 'MemberExpression' || 
                    statement.object.type === 'CallExpression') {
                    transformRunStatement(statement.object, cmap);
                } else if (cmap[statement.object.name] || 
                           statement.object.name === 's' ||
                           statement.object.name === 'm') {
                    statement.object = {
                        type: 'MemberExpression',
                        object: {
                            type: 'Identifier',
                            name: 'c'
                        },
                        property: {
                            type: 'Identifier',
                            name: statement.object.name
                        }  
                    };
                };
               break;
            case 'ForStatement':
                transformRunStatement(statement.init, cmap);
                transformRunStatement(statement.test, cmap);
                transformRunStatement(statement.update, cmap);
                transformRunStatement(statement.body, cmap);
                break;
            case 'SwitchStatement':
               statement.cases.forEach(function(c, i){
                  transformRunStatement(c, cmap);
               });
                break;
            case 'SwitchCase':
               statement.consequent.forEach(function(c, i){
                   transformRunStatement(c, cmap);
               });
                break;
            case 'IfStatement':
                transformRunStatement(statement.test, cmap);
                transformRunStatement(statement.consequent, cmap);
                if (statement.alternate) {
                    transformRunStatement(statement.alternate, cmap);   
                }
                break;
            case 'Identifier':
                if (cmap[statement.name] || 
                    statement.name === 's' ||
                    statement.name === 'm') {
                    statement.type = 'MemberExpression';
                    statement.object = {
                        type: 'Identifier',
                        name: 'c'
                    };
                    statement.property = {
                        type: 'Identifier',
                        name: statement.name
                    };
                }
                break;
            case 'ObjectExpression':
                statement.properties.forEach(function(prop, i) {
                    transformRunStatement(prop.key, cmap);
                    transformRunStatement(prop.value, cmap);
                });
            case 'UpdateExpression':
            case 'Literal':
                break;
        }
    }

    var transformRunStatements = function (block, cmap, async) {
        var statements;
        var params = [{type:'Identifier', name:'c'}];
        if (async) {
            params.push({type:'Identifier', name:'complete'})
        }

        if (block.type !== 'BlockStatement') {
            statements = [ block ];
        } else {
            statements = block.body;
        }

        statements.forEach(function (statement, index) {
            transformRunStatement(statement, cmap);
        });

        return {
            type: 'FunctionExpression',
            params: params,
            body: { type: 'BlockStatement', body: [block] }
        };
    }

    var transformExpression = function (expression, cmap, right) {
        if (expression.type === 'BinaryExpression' || 
            expression.type === 'LogicalExpression') {
            if (!expression.operator || !omap[expression.operator]) {
                throw 'syntax error: operator ' + expression.operator + ' not supported'
            } else  {
                expression.type = 'CallExpression';
                var leftExpression = expression.left;
                var rightExpression = expression.right;
                if (leftExpression.type == "Literal") {
                    leftExpression = expression.right;
                    rightExpression = expression.left;
                }

                if (cmap[leftExpression]) {
                    expression.callee = {
                        type: 'MemberExpression',
                        object: {
                            type: 'Identifier',
                            name: 'c'    
                        },
                        property: {
                            type: 'MemberExpression',
                            object: leftExpression,
                            property: {
                                type: 'Identifier',
                                name: omap[expression.operator]
                            }
                        }
                    };
                } else {
                    expression.callee = {
                        type: 'MemberExpression',
                        object: leftExpression,
                        property: {
                            type: 'Identifier',
                            name: omap[expression.operator]
                        }
                    };
                }

                expression['arguments'] = [rightExpression];
                transformExpression(leftExpression, cmap);
                transformExpression(rightExpression, cmap);
                delete(expression['operator']);
                delete(expression['left']);
                delete(expression['right']);
            } 
        } else if (expression.type === 'UnaryExpression') {
            if (!expression.operator || !omap['u' + expression.operator]) {
                throw 'syntax error: operator ' + expression.operator + ' not supported'
            } else  {
                expression.type = 'CallExpression';
                expression.callee = {
                    type: 'MemberExpression',
                    object: expression.argument,
                    property: {
                        type: 'Identifier',
                        name: omap['u' + expression.operator]
                    }
                };
                transformExpression(expression.argument, cmap);
                expression['arguments'] = [];
                delete(expression['operator']);
            }
        } else if (expression.type === 'MemberExpression') {
            if (expression.object.type === 'MemberExpression' || 
                expression.object.type === 'CallExpression') {
                transformExpression(expression.object, cmap);
            } else if (cmap[expression.object.name]) {
                expression.object = {
                    type: 'MemberExpression',
                    object: {
                        type: 'Identifier',
                        name: 'c'
                    },
                    property: {
                        type: 'Identifier',
                        name: expression.object.name
                    }  
                };
            }
        } else if (expression.type === 'CallExpression') {
            expression['arguments'].forEach(function (argument, idx) {
                transformExpression(argument, cmap);
            });
                    
            if (expression.callee.type === 'MemberExpression' && expression.callee.object.name === 's') {
                expression.callee.object = {
                    type: 'MemberExpression',
                    property: expression.callee.object,
                    object: {
                        type: 'Identifier',
                        name: 'c'
                    }
                }; 
            }
        } else if (expression.type === 'LabeledStatement') {
            expression.type = 'CallExpression';
            var functionName;
            if (expression.label.name === 'whenAll') {
                functionName = 'all';
            } else if (expression.label.name === 'whenAny') {
                functionName = 'any';
            } else {
                throw 'syntax error: label ' + expression.label.name + ' unexpected';   
            }

            expression.callee = {
                type: 'Identifier',
                name: functionName
            };

            expression['arguments'] = transformExpressions(expression.body, cmap).elements;
            delete(expression['label']);
            delete(expression['body']);

        } else if (expression.type !== 'Literal' && expression.type !== 'Identifier') {
            throw 'syntax error: expression type ' + expression.type + ' unexpected';
        }

        return expression;
    }

    var transformExpressions = function (block, cmap) {
        var statements;
        if (block.type === 'ExpressionStatement') {
            if (block.expression.type === 'SequenceExpression') {
                statements = block.expression.expressions;             
            } else {
                statements = [ block.expression ]; 
            }
        } else if (block.type === 'BlockStatement') {
            statements = block.body.map(function (s, v) { 
                if (s.type === 'ExpressionStatement') {
                    return s.expression; 
                } else if (s.type === 'LabeledStatement') {
                    return s; 
                } else {
                    throw 'syntax error: statment type ' + s.type + ' unexpected';
                }
            }, []);
        } else {
            throw 'syntax error: statement type ' + block.type + ' unexpected';
        }

        statements.forEach(function (expression, index) {
            if (expression.type !== 'AssignmentExpression') {
                transformExpression(expression, cmap);
            }
            else {
                cmap[expression.left.name] = true;
                expression.left = {
                    type: 'MemberExpression',
                    object: {
                        type: 'Identifier',
                        name: 'c'    
                    },
                    property: expression.left
                };

                transformExpression(expression.right, cmap);
            }
        });

        return {
            'type': 'ArrayExpression',
            'elements': statements
        };
    }

    var filterContext = function (block) {
        var statements = [];
        block.body.forEach(function (statement, index) {
            if (statement.type !== 'LabeledStatement') {
                statements.push(statement);
            }
        });

        return statements;
    }

    var transformRuleset = function (block) {
        var rules = [];
        var currentRule = null;
        var cmap;
        block.body.forEach(function (statement, index) {
            if (statement.type === 'LabeledStatement') {
                if (statement.label.name === 'whenAll') {
                    cmap = {};
                    currentRule = {
                        type: 'ObjectExpression',
                        properties: [{
                            type: 'Property',
                            key: { type: 'Identifier', name: 'whenAll' },
                            value: transformExpressions(statement.body, cmap)
                        }]
                    };
                    rules.push(currentRule);
                } else if (statement.label.name === 'whenAny') {
                    cmap = {};
                    currentRule = {
                        type: 'ObjectExpression',
                        properties: [{
                            type: 'Property',
                            key: { type: 'Identifier', name: 'whenAny' },
                            value: transformExpressions(statement.body, cmap)
                        }]
                    };
                    rules.push(currentRule);
                } else if (statement.label.name === 'whenStart') {
                    rules.push({
                        type: 'ObjectExpression',
                        properties: [{
                            type: 'Property',
                            key: { type: 'Identifier', name: 'whenStart' },
                            value: transformStartStatements(statement.body)
                        }]
                    });
                } else if (statement.label.name === 'run') {
                    currentRule.properties.push({
                        type: 'Property',
                        key: { type: 'Identifier', name: 'run' },
                        value: transformRunStatements(statement.body, cmap)
                    });
                } else if (statement.label.name === 'runAsync') {
                    currentRule.properties.push({
                        type: 'Property',
                        key: { type: 'Identifier', name: 'run' },
                        value: transformRunStatements(statement.body, cmap, true)
                    });
                } else if ((statement.label.name === 'pri') ||
                          (statement.label.name === 'count') ||
                          (statement.label.name === 'cap')) {
                    currentRule.properties.push({
                        type: 'Property',
                        key: { type: 'Identifier', name: statement.label.name },
                        value: statement.body.expression
                    })
                } else {
                    throw 'syntax error: whenAll, run, whenStart labels expected';
                }
            }
        });

        var body = filterContext(block);
        body.push({
            type: 'ReturnStatement',
            argument: {
                type: 'ArrayExpression',
                elements: rules,
            }
        });

        var program = {
            type: 'Program',
            body: [{
                type: 'FunctionExpression',
                id: null,
                params: [],
                body: {
                    type: 'BlockStatement',
                    body: body
                }
            }]
        }

        var func = ec.generate(program, {
            format: {
                indent: {
                    style: '  '
                }
            }
        });
        return func;
    }

    var transformState = function(block) {
        var triggers = [];
        var states = null;
        var currentTrigger = null;
        var cmap;
        block.body.forEach(function (statement, index) {
            if (statement.type === 'LabeledStatement') {
                if (statement.label.name === 'to') {
                    cmap = {};
                    currentTrigger = {
                        type: 'ObjectExpression',
                        properties: [{
                            type: 'Property',
                            key: { type: 'Identifier', name: 'to' },
                            value: statement.body.expression
                        }]
                    };

                    triggers.push(currentTrigger);
                } else if (statement.label.name === 'whenAll') {
                    currentTrigger.properties.push({
                        type: 'Property',
                        key: { type: 'Identifier', name: 'whenAll' },
                        value: transformExpressions(statement.body, cmap)
                    });
                } else if (statement.label.name === 'whenAny') {
                    currentTrigger.properties.push({
                        type: 'Property',
                        key: { type: 'Identifier', name: 'whenAny' },
                        value: transformExpressions(statement.body, cmap)
                    });
                } else if (statement.label.name === 'run') {
                    currentTrigger.properties.push({
                        type: 'Property',
                        key: { type: 'Identifier', name: 'run' },
                        value: transformRunStatements(statement.body, cmap)
                    });
                } else if (statement.label.name === 'runAsync') {
                    currentTrigger.properties.push({
                        type: 'Property',
                        key: { type: 'Identifier', name: 'run' },
                        value: transformRunStatements(statement.body, cmap, true)
                    });
                } else if ((statement.label.name === 'pri') ||
                          (statement.label.name === 'count') ||
                          (statement.label.name === 'cap')) {
                    currentTrigger.properties.push({
                        type: 'Property',
                        key: { type: 'Identifier', name: statement.label.name },
                        value: statement.body.expression
                    })
                } else {
                    if (!states) {
                        states = [];
                        triggers.push({
                            type: 'ObjectExpression',
                            properties: states
                        });
                    }

                    states.push({
                        type: 'Property',
                        key: { type: 'Identifier', name: statement.label.name },
                        value: transformState(statement.body)
                    });
                }
            }
        });

        return {
            'type': 'ArrayExpression',
            'elements': triggers
        };
    }

    var transformStates = function (block) {
        var states = [];
        block.body.forEach(function (statement, index) {
            if (statement.type === 'LabeledStatement') {
                if (statement.label.name === 'whenStart') {
                    states.push({
                        type: 'Property',
                        key: { type: 'Identifier', name: 'whenStart' },
                        value: transformStartStatements(statement.body)
                    });
                } else {
                    states.push({
                        type: 'Property',
                        key: { type: 'Identifier', name: statement.label.name },
                        value: transformState(statement.body)
                    });
                } 
            }
        });

        return states;
    }

    var transformStatechart = function (block) {
        var body = filterContext(block);
        body.push({
            type: 'ReturnStatement',
            argument: {
                type: 'ObjectExpression',
                properties: transformStates(block)
            }
        });

        var program = {
            type: 'Program',
            body: [{
                type: 'FunctionExpression',
                id: null,
                params: [],
                body: {
                    type: 'BlockStatement',
                    body: body
                }
            }]
        }
        
        var func = ec.generate(program, {
            format: {
                indent: {
                    style: '  '
                }
            }
        });
        return func;
    }

    var transformCondition = function(block) {
        var cmap = {};
        var currentCondition = {
            type: 'ObjectExpression',
            properties: []
        };

        if (block.type === 'ExpressionStatement') {
            currentCondition.properties.push({
                type: 'Property',
                key: { type: 'Identifier', name: 'whenAll' },
                value: transformExpressions(block, cmap)
            });
        } else {
            block.body.forEach(function (statement, index) {
                if (statement.type === 'LabeledStatement') {
                    if (statement.label.name === 'whenAll') {
                        currentCondition.properties.push({
                            type: 'Property',
                            key: { type: 'Identifier', name: 'whenAll' },
                            value: transformExpressions(statement.body, cmap)
                        });
                    } else if (statement.label.name === 'whenAny') {
                        currentCondition.properties.push({
                            type: 'Property',
                            key: { type: 'Identifier', name: 'whenAny' },
                            value: transformExpressions(statement.body, cmap)
                        });
                    } else if ((statement.label.name === 'pri') ||
                              (statement.label.name === 'count') ||
                              (statement.label.name === 'cap')) {
                        currentCondition.properties.push({
                            type: 'Property',
                            key: { type: 'Identifier', name: statement.label.name },
                            value: statement.body.expression
                        });
                    } else {
                        throw 'syntax error: whenAll, pri, count or cap labels expected';   
                    }
                }
            });
        }

        return currentCondition;
    }

    var transformStage = function(block, name) {
        var triggers = [];
        var currentStage = {
            type: 'ObjectExpression',
            properties: []
        };

        block.body.forEach(function (statement, index) {
            if (statement.type === 'LabeledStatement') {
                if (statement.label.name === 'run') {
                    currentStage.properties.push({
                        type: 'Property',
                        key: { type: 'Identifier', name: 'run' },
                        value: transformRunStatements(statement.body, {})
                    });
                } else if (statement.label.name === 'runAsync') {
                    currentStage.properties.push({
                        type: 'Property',
                        key: { type: 'Identifier', name: 'run' },
                        value: transformRunStatements(statement.body, {}, true)
                    });
                } else if (statement.label.name === 'self') {
                    currentStage.properties.push({
                        type: 'Property',
                        key: { type: 'Identifier', name: name },
                        value: transformCondition(statement.body) 
                    });
                } else {
                    currentStage.properties.push({
                        type: 'Property',
                        key: { type: 'Identifier', name: statement.label.name },
                        value: transformCondition(statement.body) 
                    });
                }
            }
        });

        return currentStage;
    }

    var transformStages = function (block) {
        var stages = [];
        block.body.forEach(function (statement, index) {
            if (statement.type === 'LabeledStatement') {
                if (statement.label.name === 'whenStart') {
                    stages.push({
                        type: 'Property',
                        key: { type: 'Identifier', name: 'whenStart' },
                        value: transformStartStatements(statement.body)
                    });
                } else {
                    stages.push({
                        type: 'Property',
                        key: { type: 'Identifier', name: statement.label.name },
                        value: transformStage(statement.body, statement.label.name)
                    });
                }
            }
        });

        return stages;
    }

    var transformFlowchart = function (block) {
        var body = filterContext(block);
        body.push({
            type: 'ReturnStatement',
            argument: {
                type: 'ObjectExpression',
                properties: transformStages(block)
            }
        });

        var program = {
            type: 'Program',
            body: [{
                type: 'FunctionExpression',
                id: null,
                params: [],
                body: {
                    type: 'BlockStatement',
                    body: body
                }
            }]
        }
        
        var func = ec.generate(program, {
            format: {
                indent: {
                    style: '  '
                }
            }
        });
        return func;
    }

    var argsToArray = function(args, array) {
        array = array || [];
        for (var i = 0; i < args.length; ++i) {
            array.push(args[i]);
        }

        return array;
    } 

    var lexp = function (op, terms) {
        var that = {};
        var alias;
        
        that.and = function () {
            if (op === '$and') {
                argsToArray(arguments, terms);
                return that;
            } else {
                var newTerms = [that];
                argsToArray(arguments, newTerms);
                return and.apply(this, newTerms);
            }
        };

        that.or = function()  {
            if (op === '$or') {
                argsToArray(arguments, terms);
                return that;
            } else {
                var newTerms = [that];
                argsToArray(arguments, newTerms);
                return or.apply(this, newTerms);
            }
        };

        that.setAlias = function(name) {
            alias = name;
            return that;
        }

        that.define = function(proposedAlias) {
            var definitions = [];
            for (var i = 0; i < terms.length; ++ i) {
                definitions.push(terms[i].define());
            }

            var newDefinition = {};
            newDefinition[op] = definitions;
            var aliasedDefinition = {};
            if (alias) {
                aliasedDefinition[alias] = newDefinition;
            } else if (proposedAlias) {
                aliasedDefinition[proposedAlias] = newDefinition;
            } else {
                aliasedDefinition = newDefinition;
            }
            
            return aliasedDefinition;
        };

        return that;
    };

    var term = function (type, left, sid) {
        var op;
        var right;
        var alias;
        var that;
        var sid;

        var gt = function (rvalue) {
            op = '$gt';
            right = rvalue;
            return that;
        };

        var gte = function (rvalue) {
            op = '$gte';
            right = rvalue;
            return that;
        };

        var lt = function (rvalue) {
            op = '$lt';
            right = rvalue;
            return that;
        };

        var lte = function (rvalue) {
            op = '$lte';
            right = rvalue;
            return that;
        };

        var eq = function (rvalue) {
            op = '$eq';
            right = rvalue;
            return that;
        };

        var neq = function (rvalue) {
            op = '$neq';
            right = rvalue;
            return that;
        };

        var matches = function (rvalue) {
            op = '$mt';
            right = rvalue;
            return that;
        };

        var imatches = function (rvalue) {
            op = '$imt';
            right = rvalue;
            return that;
        };

        var allItems = function (rvalue) {
            op = '$iall';
            right = rvalue;
            return that;
        };

        var anyItem = function (rvalue) {
            op = '$iany';
            right = rvalue;
            return that;
        };

        var ex = function () {
            op = '$ex';
            right  = 1;
            return that;
        };

        var nex = function () {
            op = '$nex';
            right  = 1;
            return that;
        };

        var setAlias = function(name) {
            alias = name;
            return that;
        }

        var innerAnd = function () {
            var terms = [that];
            argsToArray(arguments, terms);
            return and.apply(this, terms);
        };

        var innerOr = function () {
            var terms = [that];
            argsToArray(arguments, terms);
            return or.apply(this, terms);
        };

        var innerAdd = function () {
            var idioms = [that];
            argsToArray(arguments, idioms);
            return add.apply(this, idioms);
        }

        var innerSub = function (rvalue) {
            var idioms = [that];
            argsToArray(arguments, idioms);
            return sub.apply(this, idioms);
        }

        var innerMul = function (rvalue) {
            var idioms = [that];
            argsToArray(arguments, idioms);
            return mul.apply(this, idioms);
        }

        var innerDiv = function (rvalue) {
            var idioms = [that];
            argsToArray(arguments, idioms);
            return div.apply(this, idioms);
        }

        var define = function (proposedAlias) {
            if (!op && (type === '$s')) {
                throw 'syntax error: s cannot be an expression rvalue';
            }

            var localLeft = left;
            var localType = type;
            if (type === '$sref') {
                localType = '$s';
                if (sid) {
                    localLeft = {name: left, id: sid};
                }
            }

            var newDefinition = {};
            if (!op) {
                newDefinition[localType] = localLeft;
            } else {
                var rightDefinition = right;
                if (typeof(right) === 'object' && right !== null) {
                    rightDefinition = right.define();
                }

                if (op === '$eq') {
                    newDefinition[localLeft] = rightDefinition;
                } else {
                    var innerDefinition = {};
                    innerDefinition[localLeft] = rightDefinition;
                    newDefinition[op] = innerDefinition;
                }

                if (localType !== '$m' && localType !== '$s' && localType !== '$i') {
                    throw 'syntax error: ' + localType + ' cannot be an expression lvalue';
                }

                newDefinition = localType === '$s' ? {$and: [newDefinition, {$s: 1}]}: newDefinition;
            }

            var aliasedDefinition = {};
            if (alias) {
                aliasedDefinition[alias] = newDefinition;
            } else if (proposedAlias) {
                aliasedDefinition[proposedAlias] = newDefinition;
            } else {
                aliasedDefinition = newDefinition;
            }
            
            return aliasedDefinition;
        };

        that = r.createProxy(
            function(name) {
                switch (name) {
                    case 'gt':
                        return gt;
                    case 'gte':
                        return gte;
                    case 'lt':
                        return lt;
                    case 'lte':
                        return lte;
                    case 'eq':
                        return eq;
                    case 'neq':
                        return neq;
                    case 'matches':
                        return matches;
                    case 'imatches':
                        return imatches;
                    case 'allItems':
                        return allItems;
                    case 'anyItem':
                        return anyItem;
                    case 'ex':
                        return ex;
                    case 'nex':
                        return nex;
                    case 'and':
                        return innerAnd;
                    case 'or':
                        return innerOr;
                    case 'add':
                        return innerAdd;
                    case 'sub':
                        return innerSub;
                    case 'mul':
                        return innerMul;
                    case 'div':
                        return innerDiv;
                    case 'setAlias':
                        return setAlias;
                    case 'define':
                        return define;
                    case 'push':
                        return false;
                    default:
                        if (left) {
                            return term(type, left + '.' + name);
                        } else {
                            return term(type, name);
                        }
                }
            },
            function(name, value) {
                return;
            }
        );

        return that;
    };

    var aexp = function (op, idioms) {
        var that = {};
        var alias;
        
        that.add = function () {
            if (op === '$add') {
                argsToArray(arguments, idioms);
                return that;
            } else {
                var newIdioms = [that];
                argsToArray(arguments, newIdioms);
                return add.apply(this, newIdioms);
            }
        };

        that.sub = function () {
            if (op === '$sub') {
                argsToArray(arguments, idioms);
                return that;
            } else {
                var newIdioms = [that];
                argsToArray(arguments, newIdioms);
                return sub.apply(this, newIdioms);
            }
        };

        that.mul = function () {
            if (op === '$mul') {
                argsToArray(arguments, idioms);
                return that;
            } else {
                var newIdioms = [that];
                argsToArray(arguments, newIdioms);
                return mul.apply(this, newIdioms);
            }
        };

        that.div = function () {
            if (op === '$div') {
                argsToArray(arguments, idioms);
                return that;
            } else {
                var newIdioms = [that];
                argsToArray(arguments, newIdioms);
                return div.apply(this, newIdioms);
            }
        };

        that.define = function () {
            var currentNode;
            for (var i = idioms.length - 2; i >= 0; -- i) {
                var currentLeft;
                if (idioms[i].define) {
                    currentLeft = idioms[i].define();
                } else {
                    currentLeft = idioms[i];
                }

                var innerNode;
                if (currentNode) {
                    innerNode = {$l: currentLeft, $r: currentNode};
                } else {
                    var currentRight;
                    if (idioms[i + 1].define) {
                        currentRight = idioms[i + 1].define();
                    } else {
                        currentRight =idioms[i + 1];
                    }

                    innerNode = {$l: currentLeft, $r: currentRight};
                }
                currentNode = {};
                currentNode[op] = innerNode;
            }
            return currentNode;
        };

        return that;
    };

    var rule = function(op, lexp) {
        var that = {};
        var func;
        var rulesetArray = [];

        that.define = function(name) {
            var newDefinition = {};
            if (!lexp) {
                var argRule = op;
                if (argRule.whenAll) {
                    op = 'all';
                    if (!argRule.whenAll.push) {
                        lexp = [argRule.whenAll];
                    } else {
                        lexp = argRule.whenAll;
                    }
                }
                if (argRule.whenAny) {
                    op = 'any';
                    if (!argRule.whenAny.push) {
                        lexp = [argRule.whenAny];
                    } else {
                        lexp = argRule.whenAny;
                    }
                }
                if (argRule.pri) {
                    newDefinition['pri'] = argRule.pri;
                }
                if (argRule.count) {
                    newDefinition['count'] = argRule.count;
                }
                if (argRule.cap) {
                    newDefinition['cap'] = argRule.cap;
                }
                if (argRule.run) {
                    newDefinition['run'] = argRule.run;
                }
            } 
            var expDefinition;
            var func;

            if (typeof(lexp[lexp.length - 1]) === 'function') {
                func =  lexp.pop();
            } else if (rulesetArray.length) {
                var rulesetDefinitions = {};
                for (var i = 0; i < rulesetArray.length; ++i) {
                    rulesetDefinitions[rulesetArray[i].getName()] = rulesetArray[i].define();
                }

                func =  rulesetDefinitions;
            }

            var innerDefinition = [];
            var refName;
            var expObject;
            var newArray = [];
            for (var i = 0; i < lexp.length; ++i) {
                expDefinition = lexp[i].define(refName);
                if (expDefinition['count']) {
                    newDefinition['count'] = expDefinition['count'];
                } else if (expDefinition['pri']) {
                    newDefinition['pri'] = expDefinition['pri'];
                } else if (expDefinition['cap']) {
                    newDefinition['cap'] = expDefinition['cap'];
                } else {
                    newArray.push(lexp[i]);
                }
            }

            for (var i = 0; i < newArray.length; ++i) {
                if (newArray.length > 1) {
                    refName = 'm_' + i;
                } else {
                    refName = 'm';
                }

                expObject = {};
                if (name === undefined) {
                    expDefinition = newArray[i].define(refName);   
                } else {
                    expDefinition = newArray[i].define(name + '.' + refName);      
                }

                if (expDefinition[refName + '$all']) {
                    expObject[refName + '$all'] = expDefinition[refName + '$all'];
                } else if (expDefinition[refName + '$any']) {
                    expObject[refName + '$any'] = expDefinition[refName + '$any'];
                } else if (expDefinition[refName + '$not']) {
                    expObject[refName + '$not'] = expDefinition[refName + '$not'][0][refName + '.m'];
                } else {
                    expObject = expDefinition;
                }

                innerDefinition.push(expObject);
            }
            
            if (name !== undefined) {
                newDefinition[name + op] = innerDefinition;
            } else {
                newDefinition[op] = innerDefinition;
            }   
            
            if (func) {
                newDefinition['run'] = func;
            }   
            

            return newDefinition;
        };

        return that;
    };

    var and = function () {
        var that = lexp('$and', argsToArray(arguments));
        return that;
    };

    var or = function () {
        var that = lexp('$or', argsToArray(arguments));
        return that;
    };

    var add = function () {
        var that = aexp('$add', argsToArray(arguments));
        return that;
    };

    var sub = function () {
        var that = aexp('$sub', argsToArray(arguments));
        return that;
    };

    var mul = function () {
        var that = aexp('$mul', argsToArray(arguments));
        return that;
    };

    var div = function () {
        var that = aexp('$div', argsToArray(arguments));
        return that;
    };

    var c = r.createProxy(
        function(name) {
            if (name === 's') {
                return term('$s');
            }

            return term(name);
        },
        function(name, value) {
            value.setAlias(name);
            return value;
        }
    );

    var m = r.createProxy(
        function(name) {
            return term('$m', name);
        },
        function(name, value) {
            return;
        }
    );

    var item = r.createProxy(
        function(name) {
            return term('$i', '$i')[name];
        },
        function(name, value) {
            return;
        }
    );

    var s = r.createProxy(
        function(name) {
            return term('$s', name);
        },
        function(name, value) {
            return;
        }
    );

    var all = function() {
        var that = rule('$all', argsToArray(arguments));
        return that;
    };

    var any = function() {
        var that = rule('$any', argsToArray(arguments));
        return that;
    };
        
    var none = function(exp) {
        var that = rule('$not', [exp]);
        return that;
    };

    var timeout = function(name) {
        return m.$t.eq(name);
    };

    var sref = function(sid) {
        return r.createProxy(
            function(name) {
                return term('$sref', name, sid);
            },
            function(name, value) {
                return;
            }
        )
    };

    var extend = function (obj) {        
        obj.add = add;
        obj.sub = sub;
        obj.mul = mul;
        obj.div = div;
        obj.and = and;
        obj.or = or;
        obj.c = c;
        obj.m = m;
        obj.s = s;
        obj.item = item;
        obj.all = all;
        obj.any = any;
        obj.none = none;
        obj.timeout = timeout;

        obj.pri = function(pri) {
            var that = {};
            that.define = function () {
                return {pri: pri};
            }
            return that;
        };

        obj.count = function(count) {
            var that = {};
            that.define = function () {
                return {count: count};
            }
            return that;
        };

        obj.cap = function(cap) {
            var that = {};
            that.define = function () {
                return {cap: cap};
            }
            return that;
        };
    };

    var ruleset = function () {
        var that = {};
        var rules = [];
        var startFunc;
        var name = arguments[0];

        that.getName = function () {
            return name;
        }

        that.getStart = function () {
            return startFunc;
        }  

        that.whenAll = function () {
            var newRule = rule('all', argsToArray(arguments));
            rules.push(newRule);
            return that;
        };

        that.whenAny = function () {
            var newRule = rule('any', argsToArray(arguments));
            rules.push(newRule);
            return newRule;
        };

        that.whenStart = function (func) {
            startFunc = func;
            return that;
        };

        that.define = function () {
            var newDefinition = {};
            for (var i = 0; i < rules.length; ++ i) {
                if (rules[i]) {
                    newDefinition['r_' + i] = rules[i].define();
                }
            }

            return newDefinition;
        }

        extend(that);
        rulesets.push(that);

        for (var i = 1; i < arguments.length; ++i) {
            if (typeof(arguments[i]) === 'object') {
                rules.push(rule(arguments[i]));
            } else if (typeof(arguments[i]) === 'function') {
                var ast = ep.parse('var fn = ' + arguments[i]);
                if (!ast) {
                    throw 'invalid code block. AST generation error';
                }
                if (ast.body[0].declarations[0].init.params.length > 0) {
                    startFunc = arguments[i];
                } else {
                    func = transformRuleset(ast.body[0].declarations[0].init.body);
                    eval('var fn = ' + func + '; rules = fn();');
                    rules = rules.map(function(r, val) { 
                        if (r.whenStart) {
                            startFunc = r.whenStart;
                        } else  {
                            return rule(r);
                        } 
                    }, []);
                }
            } else {
                throw 'invalid ruleset input type ' + arguments[i];
            }   
        }

        return that;
    };

    var stateTrigger = function (stateName, run, parent, triggerObject) {
        var that = {};
        var condition;

        that.getName = function() {
            return stateName;
        };

        that.whenAll = function () {
            condition = rule('all', argsToArray(arguments));
            return parent;
        };

        that.whenAny = function () {
            condition = rule('any', argsToArray(arguments));
            return parent;
        };

        that.define = function(name)  {
            if (!condition) {
                if (!run) {
                    return {to: stateName};
                } else {
                    return {to: stateName, run: run};
                }
            } 

            var newDefinition = condition.define(name);
            newDefinition['to'] = stateName;
            return newDefinition;
        };

        if (triggerObject && (triggerObject.whenAll || triggerObject.whenAny)) {
            condition = rule(triggerObject);
        }

        return that;
    };

    var state = function (name, parent, triggerObjects) {
        var that = {};
        var triggers = [];
        var states = [];

        that.getName = function() {
            return name;
        };

        that.to = function (stateName, func) {
            var trigger = stateTrigger(stateName, func, that);
            triggers.push(trigger);
            if (func) {
                return that;
            } else {
                return trigger;
            }
        };

        that.state = function(stateName) {
            var newState = state(stateName, that);
            states.push(newState);
            return newState;
        };

        that.end = function() {
            return parent;
        }

        that.define = function() {
            var newDefinition = {};
            for (var i = 0; i < triggers.length; ++i) {
                newDefinition['t_' + i] = triggers[i].define();
            }

            if (states.length) {
                var chart = {};
                for (var i = 0; i < states.length; ++i) {
                    chart[states[i].getName()] = states[i].define();
                }

                newDefinition['$chart'] = chart;
            }

            return newDefinition;
        };

        extend(that);

        if (triggerObjects) {
            for (var i = 0; i < triggerObjects.length; ++i) {
                var triggerStatesObject = triggerObjects[i];
                if (triggerStatesObject.to) {
                    triggers.push(stateTrigger(triggerStatesObject.to, triggerStatesObject.run, that, triggerStatesObject));
                } else {
                    for (var stateName in triggerStatesObject) {
                        if (triggerStatesObject[stateName].length) {
                            states.push(state(stateName, that, triggerStatesObject[stateName]));
                        } else {
                            states.push(state(stateName, that, [triggerStatesObject[stateName]])); 
                        }
                    }    
                }
            }
        }

        return that;
    };

    var statechart = function (name, stateObjects) {
        var that = {};
        var states = [];
        var startFunc;
        
        that.getName = function () {
            return name + '$state';
        } 

        that.getStart = function () {
            return startFunc;
        }

        that.state = function (name) {
            var s = state(name, that);
            states.push(s);
            return s;
        };

        that.whenStart = function (func) {
            startFunc = func;
        };

        that.define = function() {
            var newDefinition = {};
            for (var i = 0; i < states.length; ++i) {
                newDefinition[states[i].getName()] = states[i].define();
            }

            return newDefinition;
        }

        extend(that);
        rulesets.push(that);
        if (stateObjects) {
            if (typeof(stateObjects) === 'function') {
                var ast = ep.parse('var fn = ' + stateObjects);
                if (!ast) {
                    throw 'invalid code block. AST generation error';
                }
                
                func = transformStatechart(ast.body[0].declarations[0].init.body);
                eval('var fn = ' + func + '; stateObjects = fn();');
            } 

            for (var stateName in stateObjects) {
                if (stateName === 'whenStart') {
                    startFunc = stateObjects[stateName];
                } else {
                    if (stateObjects[stateName].length) {
                        states.push(state(stateName, that, stateObjects[stateName]));
                    } else {
                        states.push(state(stateName, that, [stateObjects[stateName]]));
                    }
                }
            }  
              
        }
        
        return that;
    };

    var stageTrigger = function (stageName, parent, triggerObject) {
        var that = {};
        var condition;

        that.getName = function() {
            return stageName;
        };

        that.whenAll = function () {
            condition = rule('all', argsToArray(arguments));
            return parent;
        };

        that.whenAny = function () {
            condition = rule('any', argsToArray(arguments));
            return parent;
        };

        that.define = function(name)  {
            if (!condition) {
                return stageName;
            } 

            return condition.define(name);
        };

        if (triggerObject) {
            condition = rule(triggerObject);
        }

        return that;
    };


    var stage = function (name, parent, triggerObjects) {
        var that = {};
        var switches = [];
        var runFunc;
        
        that.getName = function() {
            return name;
        }

        that.run = function (func) {
            runFunc = func;
            return that;
        };

        that.to = function (stageName) {
            var sw = stageTrigger(stageName, that);
            switches.push(sw);
            return sw;
        };

        that.end = function() {
            return parent;
        }

        that.define = function () {
            var newDefinition = {};
            if (runFunc) {
                newDefinition['run'] = runFunc;
            }

            var switchesDefinition = {};
            for (var i = 0; i < switches.length; ++i) {
                var switchDefinition = switches[i].define();
                if (typeof(switchDefinition) === 'string') {
                    switchesDefinition = switchDefinition;
                    break;
                }

                switchesDefinition[switches[i].getName()] = switches[i].define();
            }

            newDefinition['to'] = switchesDefinition;
            return newDefinition;
        }

        extend(that);
        if (triggerObjects) {
            for (var stageName in triggerObjects) {
                if (stageName === 'run') {
                    runFunc = triggerObjects['run'];
                } else {
                    switches.push(stageTrigger(stageName, that, triggerObjects[stageName]));
                }
            }
        }

        return that;
    };

    var flowchart = function (name, stageObjects) {
        var that = {};
        var stages = [];
        var startFunc;

        that.getName = function () {
            return name + '$flow';
        } 

        that.getStart = function () {
            return startFunc;
        }

        that.stage = function (name) {
            var s = stage(name, that);
            stages.push(s);
            return s;
        };

        that.whenStart = function (func) {
            startFunc = func;
        };

        that.define = function() {
            var newDefinition = {};
            for (var i = 0; i < stages.length; ++ i) {
                newDefinition[stages[i].getName()] = stages[i].define();
            }

            return newDefinition;
        }

        extend(that);
        rulesets.push(that);
        if (stageObjects) {
            if (typeof(stageObjects) === 'function') {
                var ast = ep.parse('var fn = ' + stageObjects);
                if (!ast) {
                    throw 'invalid code block. AST generation error';
                }
                
                func = transformFlowchart(ast.body[0].declarations[0].init.body);
                eval('var fn = ' + func + '; stageObjects = fn();');
            } 

            for (var stageName in stageObjects) {
                if (stageName === 'whenStart') {
                    startFunc = stageObjects[stageName];
                } else {
                    stages.push(stage(stageName, that, stageObjects[stageName]));
                }
            } 
        }

        return that;
    };

    var rulesets = [];

    var createHost = function(databases, stateCacheSize) {
        var definitions = {};
        for (var i = 0; i < rulesets.length; ++ i) {
            definitions[rulesets[i].getName()] = rulesets[i].define(); 
        }

        var rulesHost = d.host(databases, stateCacheSize);
        // console.log(JSON.stringify(definitions, 2, 2));
        rulesHost.registerRulesets(null, definitions);
        for (var i = 0; i < rulesets.length; ++ i) {
            if (rulesets[i].getStart()) {
                rulesets[i].getStart()(rulesHost);
            }
        }

        return rulesHost;
    } 

    var createQueue = function(rulesetName, database, stateCacheSize) {
        return d.queue(rulesetName, database, stateCacheSize);
    }

    var runAll = function(databases, port, basePath, run, stateCacheSize) {
        var rulesHost = createHost(databases, stateCacheSize);
        var app = d.application(rulesHost, port, basePath);
        if (run) {
            run(rulesHost, app);
        } else {
            app.run(); 
        }
    } 

    var ex = {
        state: state,
        statechart: statechart,
        stage: stage,
        flowchart: flowchart,
        ruleset: ruleset,
        createHost: createHost,
        createQueue: createQueue,
        runAll: runAll,
    }; 
    extend(ex);

    return ex;
}();