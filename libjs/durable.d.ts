

declare interface Json {
    [propName: string]: string | number | boolean | Json;
}

declare interface IQueue {
    post(message: Json);
    assert(fact: Json);
    retract(fact: Json);
    close();
}

declare interface IHost {
    post(rulesetName: string, message: Json);
    postBatch(rulesetName: string, messages: Json[]);
    assert(rulesetName: string, fact: Json);
    assertFacts(rulesetName: string, facts: Json[]);
    retract(rulesetName: string, fact: Json);
    retractFacts(rulesetName: string, facts: Json[]);
    getState(rulesetName: string, sid: number | string): Json;
    deleteState(rulesetName: string, sid: number | string);
    patchState(rulesetName: string, state: Json);
}

declare interface IClosure {
    s: Json;
    m: Json;
    getQueue(rulesetName: string): IQueue;
    post(rulesetName: string, message: Json);
    assert(rulesetName: string, fact: Json);
    retract(rulesetName: string, fact: Json);
    delete(rulesetName: string, sid: number | string);
    startTimer(timerName: string, duration: number, id?: number | string);
    cancelTimer(rulesetName: string, id?: number | string);
    renewActionLease();
    [propName: string]: any;
}

declare interface Expression {
    gt(value: string | number | Expression): Expression;
    gte(value: string | number | Expression): Expression;
    lt(value: string | number | Expression): Expression;
    lte(value: string | number | Expression): Expression;
    eq(value: string | number | Expression): Expression;
    neq(value: string | number | Expression): Expression;
    mt(value: string): Expression;
    ex(): Expression;
    nex(): Expression;
    and(...ex: Expression[]): Expression;
    or(...ex: Expression[]): Expression;
    add(number): Expression;
    sub(number): Expression;
    mul(number): Expression;
    div(number): Expression;
}

declare interface Model {
    [propName: string]: Expression;
}

declare interface Closure {
    [propName: string]: Expression | Model;
    s: Model;
    m: Model;
}

declare interface Action {
    (c: IClosure): void;
}

declare interface AsyncAction {
    (c: IClosure, complete: (err: string) => void): void;
}

declare interface Start {
    (host: IHost): void;
}

declare interface Rule {
    whenAll?: Expression | Expression[];
    whenAny?: Expression | Expression[];
    pri?: number;
    count?: number;
    span?: number;
    cap?: number;
    run?: Action | AsyncAction;
}

declare interface Trigger extends Rule {
    to?: string;
}

declare interface StateChart {
    whenStart?: Start;
    [propName: string]: (StateChart | Trigger)[] | Start | Trigger;
}

declare interface StageTrigger {
    whenAll?: Expression | Expression[];
    whenAny?: Expression | Expression[];
    pri?: number;
    count?: number;
    span?: number;
    cap?: number;
}

declare interface Stage {
    run?: Action | AsyncAction;
    [propName: string]: StageTrigger;
}

declare interface FlowChart {
    whenStart?: Start;
    [propName: string]: Stage | Start;
}

declare interface Database {
    host: string;
    port: number;
    password?: string;
}

declare interface IDurableBase {
    m: Model;
    s: Model;
    c: Closure;
    none(Expression): Expression;
    any(...ex: Expression[]): Expression;
    all(...ex: Expression[]): Expression;
    timeout(name: string): Expression;
    ruleset(name: string, ...rules: (Rule | Start)[]);
    statechart(name: string, states?: StateChart);
    flowchart(name: string, stages?: FlowChart);
    createQueue: {
        (rulesetName: string, db?: Database | string, stateCacheSize?: number): IQueue;
    };
    createHost: {
        (dbs?: [Database | string], stateCacheSize?: number): IHost;
    };
    runAll: {
        (dbs?: [Database | string], port?: number, basePath?: string, run?: (host: IHost, app: any) => void, stateCacheSize?: number);
    };
}

declare var d : IDurableBase;
export = d;

