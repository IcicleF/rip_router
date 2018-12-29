# Simple RIP Router

## Build

```sh
make
```

## Features

支持 RIP 协议，监视以太网数据包并按照 RIP 学习的路由表转发报文。

但是，RIP 路由表项不得大于 25 个，否则不能正确处理。