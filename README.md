## Prerequisite

- [Install Mono](http://www.mono-project.com/download/)
- [Install Xamarin Studio](http://xamarin.com/platform)
- `MONO_DIR` 환경 변수 추가 (`MONO_DIR = C:\Program Files\Mono`)
- `PATH` 에 추가 `%MONO_DIR%\bin`

## Projects

- ScriptHost - mono embedding program written in C++
- ScriptApp - csharp project to run as script

## Script Debugging

1. 환경 변수 `MONODEVELOP_SDB_TEST = Y` 를 추가.

2. pdb 대신 mdb 파일이 필요하다. 아래 그림과 같이 custom command 를 추가하면 빌드할 때마다 자동으로 mdb 가 생성된다.

![xamarin-custom-commands](http://judis.me/wordpress/wp-content/uploads/2015/08/xamarin-custom-commands.png)

3. ScriptHost/ScriptHost.sln 을 열어서 Application.cpp 의 `ENABLE_SOFT_DEBUGGING` 의 주석을 풀고 실행

4. ScriptApp/ScriptApp.sln 을 Xamarin Studio 으로 열어서 break point 을 걸고, F5 를 눌러 `Launch Soft Debugger`창이 나오면 Connect 를 눌러 디버깅을 시작한다.

## References

http://www.mono-project.com/docs/advanced/embedding/
http://www.giorgosdimtsas.net/debugging-on-embedded-mono/
https://github.com/mono/mono/blob/master/samples/embed/test-invoke.c
