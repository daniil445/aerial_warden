# Russian translation

`aerialwarden_ru_RU.ts` is the editable Qt Linguist catalogue (English sources,
Russian translations). `aerialwarden_ru_RU.qm` is its compiled runtime version.
The TS file is registered in `AerialWarden.pro` under `TRANSLATIONS`.

From the project directory, using the Qt tools in PATH:

```powershell
lupdate-pro AerialWarden.pro
lrelease translations/aerialwarden_ru_RU.ts -qm translations/aerialwarden_ru_RU.qm
```

Edit translations in Qt Linguist. Preserve placeholders such as `%1`, numbers,
addresses and protocol identifiers. New messages extracted by lupdate need to
be translated before the next lrelease run.

Russian is the default language, independent of the operating system language.
The QM file is embedded in `img.qrc` and loaded before constructing the windows.
After editing the TS file, run lrelease and rebuild the application to update
the embedded translation. Runtime language switching is not implemented.
The application name `EOS EYE` remains the same in both languages.
