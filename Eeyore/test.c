int getint();
int putint(int x);
int n;
int a[10] ;//= { 1,2,3,4,5};
int main()
{
    n = getint(); 
    if(n>10)
        return 1;
    int s;
    int i = 0;
    s = i;
    while( i < n)
    {
        a[i] = getint();
        s = s + a[i];
        i = i + 1;
    }
    putint(s);
    return 0;
}
