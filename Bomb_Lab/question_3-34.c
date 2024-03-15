int call(int x)
{
    return x+1;
}
int test(int x)
{
    int a=0,b=1,c=2,d=4,e=8,f=16,g=32;
    x=x+a+b+c+d+e+f+g;
    x=call(x);
    return x;
}
int main()
{
    int x=1;
    x=test(x);
    printf("%d",x);
    return 0;
}