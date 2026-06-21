# Google.Protobuf 运行时

本目录存放 **NuGet DLL**（3.25.3）。`RpgProto` / `RpgClient` 的 asmdef 通过 `precompiledReferences` 直接引用；DLL `.meta` 设 `isExplicitlyReferenced: 1`。

**不要**添加名为 `Google.Protobuf` 的 asmdef（会与命名空间冲突导致 CS0538）。

## 首次设置

```powershell
.\scripts\fetch_google_protobuf.ps1
```

产物：

- `Google.Protobuf.dll` — NuGet `Google.Protobuf` 3.25.3（netstandard2.0）
- `Google.Protobuf.dll.meta` — `isExplicitlyReferenced: 1`；由 RpgProto/RpgClient asmdef 的 `precompiledReferences` 引用

**不要**在 `Packages/manifest.json` 添加 UPM protobuf 包。
