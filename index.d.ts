

declare interface json {
    [propName: string]: string | number | boolean | json;
}

declare interface IQueue {
    post(message: json);
    assert(fact: json);
    retract(fact: json);
    close();
}

declare interface IHost {
    post(rulesetName: string, message: json);
    postBatch(rulesetName: string, messages: json[]);
    assert(rulesetName: string, fact: json);
    assertFacts(rulesetName: string, facts: json[]);
    retract(rulesetName: string, fact: json);
    retractFacts(rulesetName: string, facts: json[]);
    getState(rulesetName: string, sid: number | string): json;
    deleteState(rulesetName: string, sid: number | string);
    patchState(rulesetName: string, state: json);
}

declare interface IClosure {
    s: json;
    m: json;
    getQueue(rulesetName: string): IQueue;
    post(rulesetName: string, message: json);
    assert(rulesetName: string, fact: json);
    retract(rulesetName: string, fact: json);
    delete(rulesetName: string, sid: number | string);
    startTimer(rulesetName: string, duration: number, id?: number | string);
    cancelTimer(rulesetName: string, id?: number | string);
    renewActionLease();
    [propName: string]: any;
}


declare interface decorator {
    (modifier: number): decorator;
}

declare interface expression {
    gt(value: string | number | expression): expression;
    gte(value: string | number | expression): expression;
    lt(value: string | number | expression): expression;
    lte(value: string | number | expression): expression;
    eq(value: string | number | expression): expression;
    neq(value: string | number | expression): expression;
    ex(): expression;
    nex(): expression;
    and(...ex: expression[]): expression;
    or(...ex: expression[]): expression;
}

declare interface model {
    [propName: string]: expression;
}

declare interface IRule<T> {
    whenAll: {
        (ex1: expression, run: (c: IClosure) => void): T;
        (ex1: expression, run: (c: IClosure, complete: (err: string) => void) => void): T;
        (ex1: expression, ex2: expression, run: (c: IClosure) => void): T;
        (ex1: expression, ex2: expression, run: (c: IClosure, complete: (err: string) => void) => void): T;
        (ex1: expression, ex2: expression, ex3: expression, run: (c: IClosure) => void): T;
        (ex1: expression, ex2: expression, ex3: expression, run: (c: IClosure, complete: (err: string) => void) => void): T;
        (ex1: expression, ex2: expression, ex3: expression, ex4: expression, run: (c: IClosure) => void): T;
        (ex1: expression, ex2: expression, ex3: expression, ex4: expression, run: (c: IClosure, complete: (err: string) => void) => void): T;
        (ex1: expression, ex2: expression, ex3: expression, ex4: expression, ex5: expression, run: (c: IClosure) => void): T;
        (ex1: expression, ex2: expression, ex3: expression, ex4: expression, ex5: expression, run: (c: IClosure, complete: (err: string) => void) => void): T;
        (d1: decorator, ex1: expression, run: (c: IClosure) => void): T;
        (d1: decorator, ex1: expression, run: (c: IClosure, complete: (err: string) => void) => void): T;
        (d1: decorator, ex1: expression, ex2: expression, run: (c: IClosure) => void): T;
        (d1: decorator, ex1: expression, ex2: expression, run: (c: IClosure, complete: (err: string) => void) => void): T;
        (d1: decorator, ex1: expression, ex2: expression, ex3: expression, run: (c: IClosure) => void): T;
        (d1: decorator, ex1: expression, ex2: expression, ex3: expression, run: (c: IClosure, complete: (err: string) => void) => void): T;
        (d1: decorator, ex1: expression, ex2: expression, ex3: expression, ex4: expression, run: (c: IClosure) => void): T;
        (d1: decorator, ex1: expression, ex2: expression, ex3: expression, ex4: expression, run: (c: IClosure, complete: (err: string) => void) => void): T;
        (d1: decorator, ex1: expression, ex2: expression, ex3: expression, ex4: expression, ex5: expression, run: (c: IClosure) => void): T;
        (d1: decorator, ex1: expression, ex2: expression, ex3: expression, ex4: expression, ex5: expression, run: (c: IClosure, complete: (err: string) => void) => void): T;
        (d1: decorator, d2: decorator, ex1: expression, run: (c: IClosure) => void): T;
        (d1: decorator, d2: decorator, ex1: expression, run: (c: IClosure, complete: (err: string) => void) => void): T;
        (d1: decorator, d2: decorator, ex1: expression, ex2: expression, run: (c: IClosure) => void): T;
        (d1: decorator, d2: decorator, ex1: expression, ex2: expression, run: (c: IClosure, complete: (err: string) => void) => void): T;
        (d1: decorator, d2: decorator, ex1: expression, ex2: expression, ex3: expression, run: (c: IClosure) => void): T;
        (d1: decorator, d2: decorator, ex1: expression, ex2: expression, ex3: expression, run: (c: IClosure, complete: (err: string) => void) => void): T;
        (d1: decorator, d2: decorator, ex1: expression, ex2: expression, ex3: expression, ex4: expression, run: (c: IClosure) => void): T;
        (d1: decorator, d2: decorator, ex1: expression, ex2: expression, ex3: expression, ex4: expression, run: (c: IClosure, complete: (err: string) => void) => void): T;
        (d1: decorator, d2: decorator, ex1: expression, ex2: expression, ex3: expression, ex4: expression, ex5: expression, run: (c: IClosure) => void): T;
        (d1: decorator, d2: decorator, ex1: expression, ex2: expression, ex3: expression, ex4: expression, ex5: expression, run: (c: IClosure, complete: (err: string) => void) => void): T;
    };
    whenAny: {
        (ex1: expression, run: (c: IClosure) => void): T;
        (ex1: expression, run: (c: IClosure, complete: (err: string) => void) => void): T;
        (ex1: expression, ex2: expression, run: (c: IClosure) => void): T;
        (ex1: expression, ex2: expression, run: (c: IClosure, complete: (err: string) => void) => void): T;
        (ex1: expression, ex2: expression, ex3: expression, run: (c: IClosure) => void): T;
        (ex1: expression, ex2: expression, ex3: expression, run: (c: IClosure, complete: (err: string) => void) => void): T;
        (ex1: expression, ex2: expression, ex3: expression, ex4: expression, run: (c: IClosure) => void): T;
        (ex1: expression, ex2: expression, ex3: expression, ex4: expression, run: (c: IClosure, complete: (err: string) => void) => void): T;
        (ex1: expression, ex2: expression, ex3: expression, ex4: expression, ex5: expression, run: (c: IClosure) => void): T;
        (ex1: expression, ex2: expression, ex3: expression, ex4: expression, ex5: expression, run: (c: IClosure, complete: (err: string) => void) => void): T;
        (d1: decorator, ex1: expression, run: (c: IClosure) => void): T;
        (d1: decorator, ex1: expression, run: (c: IClosure, complete: (err: string) => void) => void): T;
        (d1: decorator, ex1: expression, ex2: expression, run: (c: IClosure) => void): T;
        (d1: decorator, ex1: expression, ex2: expression, run: (c: IClosure, complete: (err: string) => void) => void): T;
        (d1: decorator, ex1: expression, ex2: expression, ex3: expression, run: (c: IClosure) => void): T;
        (d1: decorator, ex1: expression, ex2: expression, ex3: expression, run: (c: IClosure, complete: (err: string) => void) => void): T;
        (d1: decorator, ex1: expression, ex2: expression, ex3: expression, ex4: expression, run: (c: IClosure) => void): T;
        (d1: decorator, ex1: expression, ex2: expression, ex3: expression, ex4: expression, run: (c: IClosure, complete: (err: string) => void) => void): T;
        (d1: decorator, ex1: expression, ex2: expression, ex3: expression, ex4: expression, ex5: expression, run: (c: IClosure) => void): T;
        (d1: decorator, ex1: expression, ex2: expression, ex3: expression, ex4: expression, ex5: expression, run: (c: IClosure, complete: (err: string) => void) => void): T;
        (d1: decorator, d2: decorator, ex1: expression, run: (c: IClosure) => void): T;
        (d1: decorator, d2: decorator, ex1: expression, run: (c: IClosure, complete: (err: string) => void) => void): T;
        (d1: decorator, d2: decorator, ex1: expression, ex2: expression, run: (c: IClosure) => void): T;
        (d1: decorator, d2: decorator, ex1: expression, ex2: expression, run: (c: IClosure, complete: (err: string) => void) => void): T;
        (d1: decorator, d2: decorator, ex1: expression, ex2: expression, ex3: expression, run: (c: IClosure) => void): T;
        (d1: decorator, d2: decorator, ex1: expression, ex2: expression, ex3: expression, run: (c: IClosure, complete: (err: string) => void) => void): T;
        (d1: decorator, d2: decorator, ex1: expression, ex2: expression, ex3: expression, ex4: expression, run: (c: IClosure) => void): T;
        (d1: decorator, d2: decorator, ex1: expression, ex2: expression, ex3: expression, ex4: expression, run: (c: IClosure, complete: (err: string) => void) => void): T;
        (d1: decorator, d2: decorator, ex1: expression, ex2: expression, ex3: expression, ex4: expression, ex5: expression, run: (c: IClosure) => void): T;
        (d1: decorator, d2: decorator, ex1: expression, ex2: expression, ex3: expression, ex4: expression, ex5: expression, run: (c: IClosure, complete: (err: string) => void) => void): T;
    };
}

declare interface IStageRule {
    whenAll: {
        (...ex: expression[]): IStage;
        (d1: decorator, ...ex: expression[]): IStage;
        (d1: decorator, d2: decorator, ...ex: expression[]): IStage;
    };
    whenAny: {
        (...ex: expression[]): IStage;
        (d1: decorator, ...ex: expression[]): IStage;
        (d1: decorator, d2: decorator, ...ex: expression[]): IStage;
    };
}

declare interface IRuleset extends IRule<IRuleset> {
    whenStart(run: (host: IHost) => void);
}

declare interface IState {
    to(stateName: string): IRule<IState>;
    to(stateName: string, run: (c: IClosure) => void): IState;
    to(stateName: string, run: (c: IClosure, complete: (err: string) => void) => void): IState;
    state(stateName: string): IState;
    end(): IStatechart;
}

declare interface IStatechart {
    state(stateName: string): IState;
    whenStart(run: (host: IHost) => void);   
}

declare interface IStage {
    run(func: (c: IClosure) => void): IStage;
    run(func: (c: IClosure, complete: (err: string) => void) => void): IStage;
    to(stageName: string): IStageRule;
    end(): IFlowchart;
}

declare interface IFlowchart {
    stage(stageName: string): IStage;
    whenStart(run: (host: IHost) => void);
}

declare interface database {
    host: string;
    port: number;
    password?: string;
}

declare interface IDurableBase {
    m: model;
    s: model;
    c: model;
    pri: decorator;
    count: decorator;
    span: decorator;
    cap: decorator;
    timeout(duration: number): expression;
    ruleset(name: string): IRuleset;
    statechart(name: string): IStatechart;
    flowchart(name: string): IFlowchart;
    createQueue: {
        (rulesetName: string): IQueue;
        (rulesetName: string, db: database | string): IQueue;
        (rulesetName: string, db: database | string, stateCacheSize: number): IQueue;
    };
    createHost: {
        (): IHost;
        (dbs: [database | string]): IHost;
        (dbs: [database | string], stateCacheSize: number): IHost;
    };
    runAll: {
        ();
        (dbs: [database | string]);
        (dbs: [database | string], port: number);
        (dbs: [database | string], port: number, basePath: string);
        (dbs: [database | string], port: number, basePath: string, run: (host: IHost, app: any) => void);
        (dbs: [database | string], port: number, basePath: string, run: (host: IHost, app: any) => void, stateCacheSize: number);
    };
}

declare var d : IDurableBase;
export = d;

