"use strict"

const net = require('net')

const PORT = 19623;

angular.module('appIndex', [])
    .controller('IndexController', ['$scope', ($scope) => {
        let connected = false;
        $scope.loggedIn = false;
        $scope.loginForm = {
            ip: "",
            name: "",
            password: "",
            repeatPwd: ""
        };
        const curLoginForm = {};

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
                    break;
                case "register":
                    if (response[key] == "ok")
                        login();
                    break;
                case "info":
                    toast(response[key]);
                    break;
                }
        }

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
            connected = false;
            $scope.loggedIn = false;
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
                        try {
                            dispatch(JSON.parse(buf));
                        } catch (e) {
                            if (e instanceof SyntaxError)
                                toast('ERROR: invalid response (2)');
                            else
                                throw e;
                        }
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
    }]);

