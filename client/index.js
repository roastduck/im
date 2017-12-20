"use strict"

const net = require('net')
const fs = require('fs');
const {dialog} = require('electron').remote;

const PORT = 19623;

angular.module('appIndex', [])
    .directive('fileChange', () => {
        return {
            restrict: 'A',
            scope: {
                fileChange: '&'
            },
            link: function link(scope, element, attrs, ctrl) {
                element.on('change', onChange);

                scope.$on('destroy', function () {
                    element.off('change', onChange);
                });

                function onChange(e) {
                    scope.fileChange({$event:e}); // call expression in file-change
                    scope.$apply();
                }
            }
        };
    })
    .controller('IndexController', ['$scope', '$timeout', ($scope, $timeout) => {
        let connected = false;
        let latest = 0; // Latest timestamp received
        let curLoginForm = {};
        let fileRecv = {};

        function init() {
            $scope.loggedIn = false;
            $scope.loginForm = { // Placing inputs in an object can effectively avoid the `$parent` issue
                ip: "",
                name: "",
                password: "",
                repeatPwd: ""
            };
            $scope.contact = [];
            $scope.contactForm = {
                active: "",
                input: ""
            };
            $scope.unread = {};
            $scope.log = {};
            $scope.chatForm = {
                input: ""
            };
            $scope.avatar = {};
            connected = false;
            latest = 0;
            curLoginForm = {};
            fileRecv = {};
        }
        init();

        const toast = function(msg) {
            $('#toast').text(msg);
            $('#toast').fadeIn(300).delay(3000).fadeOut(300);
        }

        const sock = new net.Socket();
        sock.setEncoding('ascii');

        function send(data) {
            if (!connected) {
                toast("Not connected to remote server");
                return;
            }
            const raw = JSON.stringify(data);
            sock.write(raw.split('$').join('$$') + '$.');
        };
        function dispatch(response) {
            console.log(response);
            for (let key in response)
                switch (key) {
                case "login":
                    if (response[key] == "ok") {
                        $scope.loggedIn = true;
                        $scope.$apply();
                    }
                    sync();
                    break;
                case "register":
                    if (response[key] == "ok")
                        login();
                    break;
                case "info":
                    toast(response[key]);
                    break;
                case "contact":
                    $scope.contact = response[key];
                    $scope.$apply();
                    break;
                case "income":
                    for (let i in response[key]) {
                        const msg = response[key][i];
                        msg.body = JSON.parse(msg.body);
                        latest = msg.timestamp + 1 > latest ? msg.timestamp + 1 : latest;
                        let people = msg.from == curLoginForm.name ? msg.to : msg.from;
                        if ($scope.log[people] === undefined)
                            $scope.log[people] = [];
                        if (msg.body.type == "text") {
                            $scope.log[people].push(msg);
                            if ($scope.contactForm.active != people)
                                $scope.unread[people] =
                                    ($scope.unread[people] === undefined ? 0 : $scope.unread[people]) + 1;
                        }
                        if (msg.body.type == "file") {
                            if (fileRecv[people] === undefined)
                                fileRecv[people] = {};
                            if (fileRecv[people][msg.body.id] === undefined) {
                                msg.body.blob = [];
                                msg.body.recvCnt = 0;
                                msg.body.progress = function() { // NOTE: `() => {}` function has different `this` machanism
                                    return Math.round(this.recvCnt / this.size * 100) + '%';
                                };
                                msg.body.ready = function() {
                                    return this.recvCnt == this.size;
                                };
                                fileRecv[people][msg.body.id] = msg;
                                $scope.log[people].push(msg); // Create reference
                                if ($scope.contactForm.active != people)
                                    $scope.unread[people] =
                                        ($scope.unread[people] === undefined ? 0 : $scope.unread[people]) + 1;
                            }
                            const sum = fileRecv[people][msg.body.id];
                            msg.body.data = [...atob(msg.body.data)].map(c => c.charCodeAt(0));
                            for (let i = 0; i < msg.body.data.length; i++)
                                if (sum.body.blob[i + msg.body.start] === undefined) {
                                    // Same timestamp can cause re-transmission
                                    sum.body.blob[i + msg.body.start] = msg.body.data[i];
                                    sum.body.recvCnt ++;
                                }
                            delete msg.body.start;
                            delete msg.body.data;
                        }
                    }
                    for (let people in $scope.log)
                        $scope.log[people].sort((lhs, rhs) => { return lhs.timestamp - rhs.timestamp; });
                    $scope.$apply();
                    break;
                case "log":
                    if (response[key] == "more")
                        sync();
                    break;
                case "profile":
                    $scope.avatar = response[key];
                    $scope.$apply();
                    break;
                }
        }

        function sync() {
            send({cmd: 'log', since: latest});
        };
        function login() {
            send({cmd: 'login', name: curLoginForm.name, password: curLoginForm.password});
        };
        function register() {
            send({cmd: 'register', name: curLoginForm.name, password: curLoginForm.password});
        };

        let willLogin = false;
        let willRegister = false;
        sock.on('error', (err) => {
            toast('ERROR: ' + err.message);
        });
        sock.on('close', (hasError) => {
            init();
            $scope.$apply();
        });
        sock.on('connect', () => {
            connected = true;
            if (willLogin) {
                willLogin = false;
                login();
            }
            if (willRegister) {
                willRegister = false;
                register();
            }
        });
        let buf = "";
        let escaping = false;
        sock.on('data', (data) => {
            for (let i in data)
            {
                const c = data[i];
                switch (c) {
                case '$':
                    if (escaping) {
                        buf += '$';
                        escaping = false;
                    } else
                        escaping = true;
                    break;
                case '.':
                    if (escaping) {
                        escaping = false;
                        $timeout(((buf) => {
                            return () => {
                                try {
                                    dispatch(JSON.parse(buf));
                                } catch (e) {
                                    if (e instanceof SyntaxError)
                                        toast('ERROR: invalid response (2)');
                                    else
                                        throw e;
                                }
                            };
                        })(buf));
                        buf = "";
                        break;
                    }
                    // no break
                default:
                    if (escaping)
                    {
                        escaping = false;
                        toast('ERROR: invalid response (1)');
                    }
                    buf += c;
                }
            }
        });

        $scope.login = () => {
            for (let key in $scope.loginForm)
                curLoginForm[key] = $scope.loginForm[key];
            if (connected)
                login();
            else if (!sock.connecting) {
                willLogin = true;
                sock.connect(PORT, curLoginForm.ip);
            }
        };

        $scope.register = () => {
            for (let key in $scope.loginForm)
                curLoginForm[key] = $scope.loginForm[key];
            if (curLoginForm.password != curLoginForm.repeatPwd) {
                toast("Wrong password confirmation");
                return;
            }
            if (connected)
                register();
            else if (!sock.connecting) {
                willRegister = true;
                sock.connect(PORT, curLoginForm.ip);
            }
        };

        $scope.send = () => {
            const target = $scope.contactForm.active;
            const msgs = [];

            if (curFile) {
                const BLOCK_SIZE = 4096;
                let bytesSent = 0;
                for (let i = 0; i < curFile.size; i += BLOCK_SIZE) {
                    const reader = new FileReader();
                    const stop = i + BLOCK_SIZE < curFile.size ? i + BLOCK_SIZE : curFile.size;
                    reader.onload = ((target, filename, id, start, stop, size) => {
                        return (e) => {
                            send({cmd: "chat", name: target, message: JSON.stringify({
                                type: "file",
                                id: id,
                                filename: filename,
                                start: start,
                                size: size,
                                data: btoa(String.fromCharCode(...new Uint8Array(e.target.result)))
                            })});
                            bytesSent += stop - start;
                            $scope.$apply();
                        };
                    })(target, curFile.name, curFileId, i, stop, curFile.size);
                    const blob = curFile.slice(i, stop);
                    reader.readAsArrayBuffer(blob);
                }
                msgs.push({
                    type: "file",
                    filename: curFile.name,
                    size: curFile.size,
                    progress: function() { return Math.round(bytesSent / this.size * 100) + "%"; },
                    ready: () => { return false; }
                });
                curFileEvent.target.value = '';
            }

            if ($scope.chatForm.input) {
                const msg = {type:"text", data:$scope.chatForm.input};
                send({cmd: "chat", name: target, message: JSON.stringify(msg)});
                msgs.push(msg);
                $scope.chatForm.input = "";
            }

            if ($scope.log[target] === undefined)
                $scope.log[target] = [];
            for (let i in msgs)
                $scope.log[target].push({
                    timestamp: Date.now() / 1000,
                    from: curLoginForm.name,
                    to: target,
                    body: msgs[i]
                });
        };

        $scope.addContact = () => {
            const name = $scope.contactForm.input;
            send({cmd: "contact", op: "add", name: name});
        };

        $scope.delContact = (name) => {
            send({cmd: "contact", op: "del", name: name});
        };

        $scope.logout = () => {
            sock.end();
        };

        $scope.getTime = (timestamp) => {
            return (new Date(timestamp * 1000)).toLocaleString();
        };

        $scope.switchActive = (name) => {
            $scope.contactForm.active=name;
            if ($scope.unread[name] !== undefined)
                delete $scope.unread[name];
        };

        $scope.chatKeyPress = (e) => {
            if (e.shiftKey && e.key == 'Enter')
                $scope.send();
        };

        let curFileId = 0;
        let curFile = null;
        let curFileEvent = null;
        $scope.addFile = (e) => {
            curFileEvent = e;
            if (e.target.value.length == 0)
                curFile = null;
            else {
                curFileId = Date.now() * 100 + Math.round(Math.random() * 100);
                curFile = e.target.files[0];
            }
        };
        $scope.saveFile = (f) => {
            dialog.showSaveDialog({defaultPath: f.filename}, (filename) => {
                fs.writeFile(filename, new Buffer(f.blob), (err) => {
                    if (err)
                        toast("Cannot save to " + filename);
                    else
                        toast("File " + filename + " saved");
                });
            });
        };

        $scope.changeAvatar = () => {
            const filenames = dialog.showOpenDialog({
                filters: [{name: 'Images', extensions: ['jpg', 'jpeg', 'png']}],
                properties: ['openFile']
            });
            if (!filenames)
                return;
            let prefix = '';
            if (filenames[0].endsWith(".jpg") || filenames[0].endsWith(".jpeg"))
                prefix = "data:image/jpeg;base64,";
            else
                prefix = "data:image/png;base64,";
            fs.readFile(filenames[0], (err, data) => {
                if (err) {
                    toast("Error reading image");
                    return;
                }
                const img = prefix + data.toString('base64')
                $scope.avatar[curLoginForm.name] = img;
                send({cmd: 'profile', data: img});
                $scope.$apply();
            });
        };
        $scope.getAvatar = () => {
            return curLoginForm.name ? $scope.avatar[curLoginForm.name] : '';
        };
    }]);

