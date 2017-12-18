angular.module('appIndex', [])
    .service('tcp', [function() {
        this.send = function(msg) {
            // TODO
        };
    }])
    .controller('IndexController', ['$scope', 'tcp', function($scope, tcp) {
        // TODO
    }]);

