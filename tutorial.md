Tutorial
=====
### Table of contents
* [Introduction](tutorial.md#introduction)
* [Ruleset](tutorial.md#ruleset)
* [Timers](tutorial.md#timers)
* [Inference](tutorial.md#inference)
* [Statechart](tutorial.md#statechart)
* [Actions](tutorial.md#actions)  
* [Hosting](tutorial.md#hosting)

### Introduction
In this tutorial we are going to build an expense approval process. The objectives are to walk you through the different features Durable Rules supports and to show a simple practical application of the technology. Let's start with a process description:
* An employee submits an approval request, which contains the amount and the approver
* If the expense is greater than $1000, it is automatically denied
* Otherwise, an email is sent to the approver with a link to the approval web page
* The approver selects approve\deny in the web page
* If the approver doesn't respond on time, the email is resent once 
* After retrying if the approver does not click on the web page, the request is denied  

### Ruleset
First we will define a couple of rules: 'r1' will log 'denied' to the console is a message with 'amount' greater than 1000 is sent. 'r2' will log 'approved' to the console if a message with 'amount' less than or equal to 1000 is sent. For detailed information see the [rules](/concepts.md#rules) and [expressions](/concepts.md#expressions) section in the concepts page.   
  
1. Install Durable Rules library
  1. Open a terminal and go to documents/approve
  2. Run `npm -install 'durable'`
2. Using your favorite text editor, create a test.js (documents/approve/test.js)
3. Copy\Paste the program below and save
  ```javascript
    var d = require('durable');
    d.run({
        approve: {
            r1: {
                when: { $gt: { amount: 1000 } },
                run: function(s) { console.log('denied'); }
            },
            r2: {
                when: { $lte: { amount: 1000 } },
                run: function(s) { console.log('approved'); }
            }
        }
    });   
  ```

4. Make sure redis is running (see [setup](/setup) for details)
5. Start a terminal, goto documents/approve, type node test.js
6. Open a browser and navigate to http://localhost:5000/approve/1/admin.html
7. Type the message definition in the textbox at the bottom and click the 'Post Message' button
  ```javascript
  { "id":1, "amount":500 }
  ```

8. Type the message definition in the textbox at the bottom and click the 'Post Message' button
  ```javascript
  { "id":2, "amount":1500 }
  ```

9. In the terminal window you should see
  ```
  approved
  denied
  ```

[top](tutorial.md#table-of-contents)  

### Timers
Let's add now the time dimension to our program. We are going to modify the ruleset above to start a timer when an event message with `amount < 1000` is seen. The timer then triggers an action after 5 minutes. For detailed information see the [timers](/concepts.md#timers) section in the concepts page. 

1. In your test.js file copy\paste the program below and save
  ```javascript
    var d = require('durable');
    d.run({
        approve: {
            r1: {
                when: { $gt: { amount: 1000 } },
                run: function(s) { console.log('denied'); }
            },
            r2: {
                when: { $lte: { amount: 1000 } },
                run: function(s) { s.startTimer('timeout', 300); }
            },
            r3: {
                when: { $t: 'timeout' },
                run: function(s) { console.log('timed out'); }
            }
        }
    });   
  ```

2. Make sure redis is running (see [setup](/setup) for details).
3. Clear the redis database 
  1. Open a terminal and goto `library/redis-2.8.1`
  2. Type `src/redis-cli`
  3. Type `flushdb` 
4. Start a terminal, goto `documents/approve`, type `node test.js`
5. Open a browser and navigate to http://localhost:5000/approve/1/admin.html
6. Type the message definition in the textbox at the bottom and click the 'Post Message' button
  ```javascript
  { "id":1, "amount":500 }
  ```

7. After 5 minutes, in the terminal window you should see
  ```
  timed out
  ```

[top](tutorial.md#table-of-contents)  

### Inference
We are going to make things a little more interesting by writing rules to coordinate several events using the `whenAll` and `any` expression operators. Such operators rely on rule inference for efficient evaluation. This example starts a timer when an message event with `amount < 1000` is seen. At that point it waits 5 minutes for a message event with `subject = 'approved'`. To learn more see the [event algebra](/concepts.md#event-algebra) and the [inference](/concepts.md#inference)  section in the concepts page.  

1. In your test.js file copy\paste the program below and save
  ```javascript
    var d = require('durable');
    d.run({
        approve: {
            r1: {
                whenAll: {
                    $s: { $nex: { state: 1 } }, 
                    $m: { $gt: { amount: 1000 } }
                },
                run: function(s) { console.log('denied'); }
            },
            r2: {
                whenAll: {
                    $s: { $nex: { state: 1 } }, 
                    $m: { $lte: { amount: 1000 } }
                },
                run: function(s) { 
                    s.state = 'pending';
                    s.startTimer('timeout', 300);
                    console.log('request approval');  
                }
            },
            r3: {
                whenAll: {
                    $s: { state: 'pending' },
                    $m: { subject: 'approve' } 
                },
                run: function(s) { console.log('approved'); }
            },
            r4: {
                whenAll: {
                    $s: { state: 'pending' },
                    m$any: {
                        m: { subject: 'deny' },
                        t: { $t: 'timeout' }
                    } 
                },
                run: function(s) { console.log('denied'); }
            }
        }
    });
  ```

2. Make sure redis is running (see [setup](/setup) for details).
3. Start a terminal, goto `documents/approve`, type `node test.js`
4. Clear the redis database 
  1. Open a terminal and goto `library/redis-2.8.1`
  2. Type `src/redis-cli`
  3. Type `flushdb` 
5. Open a browser and navigate to http://localhost:5000/approve/1/admin.html
6. Type the message definition in the textbox at the bottom and click the 'Post Message' button
  ```javascript
  { "id":1, "amount":500 }
  ```

7. Type the message definition in the textbox at the bottom and click the 'Post Message' button
  ```javascript
  { "id":2, "subject":"approved" }
  ```

8. If you take less than 5 minutes to complete step 7, in the terminal window you should see
  ```
  request approval
  approved
  ```

9. If you take more than 5 minutes to complete step 7, in the terminal window you should see
  ```
  request approval
  denied
  ```

[top](tutorial.md#table-of-contents)  

### Statechart
As you can see the example above has become complex, it is not easy to understand. A little bit of structure would help. A statechart is a great way of organizing reactions to events in different stages. In the code below we have four states: 'initial', which waits for a message with the amount property. 'pending', which waits for a message with subject 'approved', 'denied' or a timeout. 'approved' and 'denied' are the final outcome states. Notice how the statemachine retries a couple of times before moving to the denied state. For detailed information see the [statechart](/concepts.md#statechart)  section in the concepts page.  

1. In your test.js file copy\paste the program below and save
  ```javascript
    var d = require('durable');
    d.run({
        approve$state: {
            input: {
                deny: {
                    when: { $gt: { amount: 1000 } },
                    run: deny,
                    to: 'denied'
                },
                request: {
                    when: { $lte: { amount: 1000 } },
                    run: requestApproval,
                    to: 'pending'
                }
            },
            pending: {
                request: {
                    whenAll: {
                        $s: { $lt: { i: 3 } },
                        $m: { $t: 'timeout' }
                    },
                    run: requestApproval,
                    to: 'pending'
                },
                approve: {
                    when: { subject: 'approve' },
                    run: approve,
                    to: 'approved'
                },
                deny: {
                    whenAny: {
                        $m: { subject: 'deny' },
                        s$all: {
                            $s: { $gte: { i: 3 } },
                            $m: { $t: 'timeout' }
                        }
                    },
                    run: deny,
                    to: 'denied'
                }
            },
            denied: {
            },
            approved: {
            }
        }
    });

    function requestApproval(s) {
        if (!s.i) {
            s.i = 1;
        } else {
            ++s.i;
        } 

        s.startTimer('timeout', 300);
        console.log('request approval');
    }

    function approve(s) {
        console.log('approved');
    }

    function deny(s) {
        console.log('denied');
    }
  ```

2. Make sure redis is running (see [setup](/setup) for details).
3. Start a terminal, goto `documents/approve`, type `node test.js`
4. Clear the redis database 
  1. Open a terminal and goto `library/redis-2.8.1`
  2. Type `src/redis-cli`
  3. Type `flushdb` 
5. Open a browser and navigate to http://localhost:5000/approve/1/admin.html
6. Type the message definition in the textbox at the bottom and click the 'Post Message' button
  ```javascript
  { "id":1, "amount":500 }
  ```

7. Type the message definition in the textbox at the bottom and click the 'Post Message' button
  ```javascript
  { "id":2, "subject":"approved" }
  ```

8. If you take less than 5 minutes to complete step 7, in the terminal window you should see
  ```
  request approval
  approved
  ```

9. If you take more than 10 minutes to complete step 7, in the terminal window you should see
  ```
  request approval
  request approval
  denied
  ```

[top](tutorial.md#table-of-contents)  

### Actions
Logging to the console is not that interesting, in this section we are going to enable our rules send email messages. Aside from the synchronous actions used in previous examples to log messages to the console, we can integrate asynchronous I/O actions following the simple node.js convention. Note that we are going to build this functionality on top of the example presented in the previous [statechart](tutorial.md#statechart) section. For more details visit the [actions](/concepts.md#actions) section in the concepts page.  

1. Install nodemailer
  1. Open a terminal and go to documents/approve
  2. Run `npm -install 'nodemailer'`
2. Paste the following line to the top of the test.js file (set a valid Gmail user account and password in auth object)
  ```javascript
    var mailer = require('nodemailer');
    var transport = mailer.createTransport('SMTP',{
        service: 'Gmail',
        auth: {
            user: 'emailaccount',
            pass: 'password'
        }
    });
  ```

3. Replace the `requestApproval`, `approve` and `deny` functions in the example above with the functions below
  ```javascript
    function deny (s, complete) {
        setState(s);
        var mailOptions = {
            from: 'approval',
            to: s.from,
            subject: 'Request for ' + s.amount + ' has been denied.'
        };
        transport.sendMail(mailOptions, complete);
    }

    function requestApproval (s, complete) {
        setState(s);
        if (!s.i) {
            s.i = 1;
        } else {
            ++s.i;
        }

        s.startTimer('timeout', 300);
        var mailOptions = {
            from: 'approval',
            to: s.to,
            subject: 'Requesting approval for ' + s.amount + ' from ' + s.from + ' try #' + s.i + '.',
            html: 'Go <a href="http://localhost:5000/approve.html?session=' + s.id + '">here</a> to approve or deny.'
        };
        transport.sendMail(mailOptions, complete);
    }

    function approve (s, complete) {
        var mailOptions = {
            from: 'approval',
            to: s.from,
            subject: 'Request for ' + s.amount + ' has been approved.'
        };
        transport.sendMail(mailOptions, complete);
    }

    function setState(s) {
        if (!s.from) {
            s.from = s.getOutput().from;
            s.amount = s.getOutput().amount;
            s.to = s.getOutput().to;
        }
    }
  ```
4. Make sure redis is running (see [setup](/setup) for details).
5. Start a terminal, goto `documents/approve`, type `node test.js`
6. Clear the redis database 
  1. Open a terminal and goto `library/redis-2.8.1`
  2. Type `src/redis-cli`
  3. Type `flushdb` 
7. Open a browser and navigate to http://localhost:5000/approve/1/admin.html
8. Type the message definition in the textbox at the bottom and click the 'Post Message' button (set a real email address in the 'from' and 'to' fields:
  ```javascript
  { "id":1, "from":"youremail", "to":"youremail", "amount":500 }
  ```

9. The 'request approval' message will be sent to the 'to' email address
10. The 'request denied' message will be sent to the 'from' email address after 10 minutes

[top](tutorial.md#table-of-contents)  
### Hosting
When running the example in the previous section, you might have noticed the link in the email was broken. In this section we are going to fix that problem by hosting the 'approve' and 'deny' webpage building on top of the code described in the [actions](tutorial.md#actions) section. Durable rules relies on Node.js and gives developers the opportunity to use the Node.js app object. Note how in the code in the approval.html page defined below we raise an event by posting a message to the REST interface. To learn more visit the [web messages](/concepts.md#web-messages) section in the concepts page.

1. Install node static
  1. Open a terminal and go to documents/approve
  2. Run `npm -install 'node-static'`
2. Add the following line to the top of the test.js file
  ```javascript
  var stat = require('node-static');
  ```

3. Right after the statechart definition add the following code
  ```javascript
    // other code omitted for clarity
    var d = require('durable');
    d.run({
    // other code omitted for clarity
    }, '', null, function(host) {
        var fileServer = new stat.Server(__dirname);
        host.getApp().get('/approve.html', function (request, response) {
            request.addListener('end', function () {
                fileServer.serveFile('/approve.html', 200, {}, request, response);
            }).resume();
        });
    });
  ```

4. Using you favorite html editor create an html file documents\approve\approve.html
5. Paste the following code in the file
  ```html
  <html>
  <head>
      <title>Approval page</title>
  </head>
  <body>
  <h3> Do you approve the expense?</h3>
  <script type="text/javascript" src="http://d3js.org/d3.v3.min.js?3.1.9"></script>
  <script type="text/javascript">
  ```
  ```javascript
    function post(choice) {
        server.post('{ "id":"' + Math.floor(Math.random()*1000) + '", "subject":"' + choice + '"}', function(err) {
            if (err) {
                alert(err.responseText);
            }
            else {
                alert('OK');
            }
        });
    }

    function approve() {
        post('approve');
    }

    function deny() {
        post('deny');
    }


    function getQueryVariables(query) {
        query = query.substring(1);
        var result = {};
        var vars = query.split('&');
        var pair;
        for (var i = 0; i < vars.length; i++) {
            pair = vars[i].split('=');
            result[decodeURIComponent(pair[0])] = decodeURIComponent(pair[1]);
        }

        return result;
    }

    var query = getQueryVariables(window.location.search);
    var url = 'http://localhost:5000/approve/' + query.session;
    var server = d3.xhr(url)
            .header('content-type', 'application/json; charset=utf-8');

    var body = d3.select('body');
    body.append('input')
            .attr('type', 'button')
            .attr('id', 'approve')
            .attr('value', 'Yes')
            .on('click', approve);

    var body = d3.select('body');
    body.append('input')
            .attr('type', 'button')
            .attr('id', 'deny')
            .attr('value', 'No')
            .on('click', deny);
  ```
  ```html
  </script>
  </body>
  </html>
  ```

6. Make sure redis is running
7. Start a terminal, goto `documents/approve`, type `node test.js`
8. Clear the redis database 
  1. Open a terminal and goto `library/redis-2.8.1`
  2. Type `src/redis-cli`
  3. Type `flushdb` 
8. Open a browser and navigate to http://localhost:5000/approve/1/admin.html
10. Type the message definition in the textbox at the bottom and click the 'Post Message' button (set a real email address in the 'from' and 'to' fields:
  ```javascript
  { "id":1, "from":"youremail", "to":"youremail", "amount":500 }
  ```

11. The 'request approval' message will be sent to the 'to' email address
12. Click the link in the email message (note the link address is localhost, so you need to open the mail in the same machine as you run the service)
13. Click one of the 'yes' or 'no' buttons
14. An email message with the decision outcome will be sent to the 'from' address

[top](tutorial.md#table-of-contents)  
