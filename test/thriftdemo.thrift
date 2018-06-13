namespace cpp test.demo

service ThriftDemo
{
    string sendPing(1:string ping);
}