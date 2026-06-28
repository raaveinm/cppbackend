The `tcp::acceptor::async_accept` method is used to asynchronously wait for connections. We will use the version of this method that accepts two parameters: an executor and a handler function. This method:

asynchronously waits for a client connection;
creates a socket, passing the executor to it;
invokes the handler function, passing it the error code and the created socket.

A new `strand` object should be passed as the executor. This ensures that the resulting socket executes its handler functions sequentially and independently of the handler functions of other sockets.

```c++
net::io_context ioc;
// acceptor будет вызывать свои функции-обработчики последовательно внутри strand
tcp::acceptor acceptor{net::make_strand(ioc)};
...
acceptor.async_accept(
net::make_strand(ioc), // socket будет вызывать свои операции в своём strand
[](beast::error_code ec, tcp::socket socket) {
/* Если ec не содержит ошибки, через socket можно обмениваться данными */
});
```
To pass a method of the current object as a handler function, we use the helper function `beast::bind_front_handler`. It binds the remaining arguments to its first argument (a function or method) and returns a new handler. This handler invokes the wrapped function, passing it the bound arguments followed by its own arguments.
```c++
auto handler = boost::beast::bind_front_handler(SomeFunc, arg1, arg2);
handler(arg3, arg4, arg5); // Вызовет SomeFunc(arg1, arg2, arg3, arg4, arg5); 
```