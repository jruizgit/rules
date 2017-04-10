exports = module.exports = durableEngine = function () {
    var d = require('./durableEngine');
    var r = require('bindings')('rulesjs.node');
    var ep = require('esprima');
    var ec = require('escodegen');
    var dh = d.host();

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
        '~': 'nex',
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
               statement.cases.forEach(function(_case, idx){
                  transformStartStatement(_case);
               });
            break;
            case 'SwitchCase':
               statement.consequent.forEach(function(con, idx){
                   transformStartStatement(con);
               });
            break;
            case 'MemberExpression':
            case 'UpdateExpression':
            case 'Literal':
            case 'Identifier':
                break;
        }
    }

    var transformStartStatements = function (block, cmap) {
        //console.log(JSON.stringify(block, null, 2));
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
            type: 'ObjectExpression',
            properties: [{
                type: 'Property',
                key: { type: 'Identifier', name: 'whenStart' },
                value: {
                    type: 'FunctionExpression',
                    params: [{type:'Identifier', name:'host'}],
                    body: { type: 'BlockStatement', body: statements }
                }
            }]
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
                transformRunStatement(statement.callee, cmap);
                break;
            case 'AssignmentExpression':
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
                if (statement.object.type === 'MemberExpression') {
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
            case 'SwitchStatement':
               statement.cases.forEach(function(_case, idx){
                  transformRunStatement(_case, cmap);
               });
            break;
            case 'SwitchCase':
               statement.consequent.forEach(function(con, idx){
                   transformRunStatement(con, cmap);
               });
            break;
            case 'UpdateExpression':
            case 'Literal':
            case 'Identifier':
                break;
        }
    }

    var transformRunStatements = function (block, cmap) {
        //console.log(JSON.stringify(block, null, 2));
        var statements;
        if (block.type !== 'BlockStatement') {
            statements = [ block ];
        } else {
            statements = block.body;
        }

        statements.forEach(function (statement, index) {
            transformRunStatement(statement, cmap);
        });

        return {
            type: 'Property',
            key: { type: 'Identifier', name: 'run' },
            value: {
                type: 'FunctionExpression',
                params: [{type:'Identifier', name:'c'}],
                body: { type: 'BlockStatement', body: [block] }
            }
        };
    }

    var transformExpression = function (expression, cmap) {
        if (expression.type === 'BinaryExpression' || 
            expression.type === 'LogicalExpression') {
            if (!expression.operator || !omap[expression.operator]) {
                throw 'syntax error: operator ' + expression.operator + ' not supported'
            } else  {
                expression.type = 'CallExpression';
                if (cmap[expression.left]) {
                    expression.callee = {
                        type: 'MemberExpression',
                        object: {
                            type: 'Identifier',
                            name: 'c'    
                        },
                        property: {
                            type: 'MemberExpression',
                            object: expression.left,
                            property: {
                                type: 'Identifier',
                                name: omap[expression.operator]
                            }
                        }
                    };
                } else {
                    expression.callee = {
                        type: 'MemberExpression',
                        object: expression.left,
                        property: {
                            type: 'Identifier',
                            name: omap[expression.operator]
                        }
                    };
                }

                expression['arguments'] = [expression.right];
                transformExpression(expression.left, cmap);
                transformExpression(expression.right, cmap);
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
            if (expression.object.type === 'MemberExpression') {
                transformExpression(expression.object);
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
        } else if (expression.type !== 'Literal' && 
                   expression.type !== 'Identifier' &&
                   expression.type !== 'CallExpression') {
            throw 'syntax error: expression type ' + expression.type + ' unexpected';
        }

        return expression;
    }

    var transformExpressions = function (block, cmap) {
        //console.log(JSON.stringify(block, null, 2));
        var statements;
        if (block.type === 'ExpressionStatement') {
            statements = [ block.expression ];
        } else if (block.type === 'BlockStatement') {
            statements = block.body.map(function (s, v) { return s.expression; }, []);
        } else {
            throw 'syntax error: statement type ' + statement.type + ' unexpected';
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

        return currentRule = {
            type: 'ObjectExpression',
            properties: [{
                type: 'Property',
                key: { type: 'Identifier', name: 'whenAll' },
                value: {
                    'type': 'ArrayExpression',
                    'elements': statements
                }
            }]
        };
    }

    var transformRuleset = function (block) {
        var rules = [];
        var currentRule = null;
        var cmap;
        block.body.forEach(function (statement, index) {
            if (statement.type !== 'LabeledStatement') {
                throw 'syntax error: statement type ' + statement.type + ' unexpected';
            }

            if (statement.label.name === 'whenAll') {
                cmap = {};
                currentRule = transformExpressions(statement.body, cmap);
                rules.push(currentRule);
            } else if (statement.label.name === 'whenStart') {
                rules.push(transformStartStatements(statement.body));
            } else if (statement.label.name === 'run') {
                currentRule.properties.push(transformRunStatements(statement.body, cmap));
            } else if ((statement.label.name === 'pri') ||
                      (statement.label.name === 'count')||
                      (statement.label.name === 'span')||
                      (statement.label.name === 'cap')) {
                currentRule.properties.push({
                    type: 'Property',
                    key: { type: 'Identifier', name: statement.label.name },
                    value: statement.body.expression
                })
            } else {
                throw 'syntax error: whenAll, run, whenStart labels expected';
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
                    body: [{
                        type: 'ReturnStatement',
                        argument: {
                            type: 'ArrayExpression',
                            elements: rules,
                        }
                    }]
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
        console.log(func);
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

    var term = function (type, left) {
        var op;
        var right;
        var alias;
        var that;

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

        var mt = function (rvalue) {
            op = '$mt';
            right = rvalue;
            return that;
        };

        var matches = function (rvalue) {
            op = '$mt';
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

        var define = function (proposedAlias) {
            var newDefinition = {};
            if (!op) {
                newDefinition[type] = left;
            } else {
                var rightDefinition = right;
                if (typeof(right) === 'object' && right !== null) {
                    rightDefinition = right.define();
                }

                if (op === '$eq') {
                    newDefinition[left] = rightDefinition;
                } else {
                    var innerDefinition = {};
                    innerDefinition[left] = rightDefinition;
                    newDefinition[op] = innerDefinition;
                }

                newDefinition = type === '$s' ? {$and: [newDefinition, {$s: 1}]}: newDefinition;
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
                    case 'mt':
                        return mt;
                    case 'matches':
                        return matches;
                    case 'ex':
                        return ex;
                    case 'nex':
                        return nex;
                    case 'and':
                        return innerAnd;
                    case 'or':
                        return innerOr;
                    case 'setAlias':
                        return setAlias;
                    case 'define':
                        return define;
                    case 'push':
                        return false;
                    default:
                        return term(type, left + '.' + name);
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

    var idiom = function (type, parentName) {
        var that;
        var op;
        var right = [];
        var left;
        var sid;

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

        var refId = function (refid) {
            sid = refid;
            return r.createProxy(
                function(name) {
                    left = name;
                    return that;
                },
                function(name, value) {
                    return;
                }
            );
        }; 

        var define = function () {
            var newDefinition;
            if (sid) {
                newDefinition = {name: left, id: sid};
            } else {
                newDefinition = left;
            }
            var leftDefinition = {};
            leftDefinition[type] = newDefinition;
            return leftDefinition;
        }

        that = r.createProxy(
            function(name) {
                switch (name) {
                    case 'add':
                        return innerAdd;
                    case 'sub':
                        return innerSub;
                    case 'mul':
                        return innerMul;
                    case 'div':
                        return innerDiv;
                    case 'refId':
                        return refId;
                    case 'define':
                        return define;
                    default:
                        left = left + '.' + name;        
                        return that;
                }
            },
            function(name, value) {
                return;
            }
        );

        return r.createProxy(
            function(name) {
                if (name === 'refId') {
                    return refId;
                }


                left = name;
                return that;
            },
            function(name, value) {
                return;
            }
        );
    }

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
                if (argRule.span) {
                    newDefinition['span'] = argRule.span;
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
                } else if (expDefinition['span']) {
                    newDefinition['span'] = expDefinition['span'];
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
                return idiom('$s');
            }

            return idiom(name);
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

    var s = r.createProxy(
        function(name) {
            return term('$s', name);
        },
        function(name, value) {
            return;
        }
    );

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

        obj.all = function() {
            var that = rule('$all', argsToArray(arguments));
            return that;
        };

        obj.any = function() {
            var that = rule('$any', argsToArray(arguments));
            return that;
        };
        
        obj.none = function(exp) {
            var that = rule('$not', [exp]);
            return that;
        }

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

        obj.span = function(span) {
            var that = {};
            that.define = function () {
                return {span: span};
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

        obj.timeout = function(name) {
            return m.$t.eq(name);
        }
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

        that.timeout = function(name) {
            return m.$t.eq(name);
        }

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
                    triggers.push(stateTrigger(triggerStatesObject.to, null, that, triggerStatesObject));
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
                var switchDefinition = switches[i].define('');
                if (typeof(switchDefinition) === 'string') {
                    switchesDefinition = switchDefinition;
                    break;
                }

                switchesDefinition[switches[i].getName()] = switches[i].define('');
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
            //console.log(rulesets[i].getName() + ' ***** ' + JSON.stringify(definitions[rulesets[i].getName()]))
        }

        var rulesHost = d.host(databases, stateCacheSize);
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