using System.Text.RegularExpressions;
using System.Security.Cryptography;
using System.Text.Json;
using Mutagen.Bethesda;
using Mutagen.Bethesda.Plugins;
using Mutagen.Bethesda.Plugins.Records;
using Mutagen.Bethesda.Skyrim;

if(args.Length>0 && args[0]=="--sources") { Sources.Extract(args.Skip(1).ToArray()); return; }
var root = Path.GetFullPath(args.Length > 0 ? args[0] : ".");
var output = Path.Combine(root, "dist", "SkillPractice");
Directory.CreateDirectory(output);
var names = Regex.Matches(File.ReadAllText(Path.Combine(root,"config","kid","SPO_WeaponClass_KID.ini")), @"SPO_WeapClass_[A-Za-z0-9_]+")
    .Select(x=>x.Value).Distinct().Order(StringComparer.Ordinal).ToArray();
if(names.Length != 22) throw new Exception($"Expected 22 keywords, got {names.Length}");
var mod = new SkyrimMod(ModKey.FromNameAndExtension("SkillPractice.esp"), SkyrimRelease.SkyrimSE);
mod.ModHeader.Flags |= SkyrimModHeader.HeaderFlag.Small;
mod.ModHeader.MasterReferences.Add(new MasterReference { Master = ModKey.FromNameAndExtension("Skyrim.esm") });
uint id=0x800;
var rows=new List<object>();
foreach(var name in names) {
    var kw = new Keyword(new FormKey(mod.ModKey,id++),SkyrimRelease.SkyrimSE) { EditorID = name };
    mod.Keywords.Add(kw); rows.Add(new {editorID=name,localID=$"0x{kw.FormKey.ID:X6}"});
}
var path=Path.Combine(output,"SkillPractice.esp");
mod.WriteToBinary(path);
File.WriteAllText(Path.Combine(root,"tools","esp-gen","keywords.json"),JsonSerializer.Serialize(rows,new JsonSerializerOptions{WriteIndented=true})+"\n");
Console.WriteLine($"Wrote {path}: 22 keywords; SHA256 {Convert.ToHexString(SHA256.HashData(File.ReadAllBytes(path)))}");
