
def run(ruleset_definitions = None, databases = ['/tmp/redis.sock'], start = None):
    import engine
    import interface

    main_host = engine.Host(ruleset_definitions, databases)
    if start:
        start(main_host)

    main_app = interface.Application(main_host)
    main_app.run()







