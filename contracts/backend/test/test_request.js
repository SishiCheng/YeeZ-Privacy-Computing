const { BN, constants, expectEvent, expectRevert } = require('openzeppelin-test-helpers');
const { expect } = require('chai');
const YZDataRequestFactory = artifacts.require("YZDataRequestFactory");
const YZDataRequest = artifacts.require("YZDataRequest");
const YZDataFactory = artifacts.require("YZDataFactory");
const YZData = artifacts.require("YZData");
const CertifiedUsers = artifacts.require("CertifiedUsers");
const TestProgramStore = artifacts.require("TestProgramStore");

contract('TESTYZDataRequest', (accounts) => {

  let addr = {};
  let factory = {}
  let data = {}
  let data_hash = accounts[4]
  let drequest = {}
  let cert = {}
  let program_proxy ={}
  let pkey;
  let program_hash = {}

	context('init', async () => {
    it('init', async()=>{
      cert = await CertifiedUsers.deployed();
      program_proxy = await TestProgramStore.deployed();

      data_factory = await YZDataFactory.deployed();
      tx = await data_factory.createYZData(data_hash, "test", "test desc", "0x00", "test env", 0, cert.address, program_proxy.address);
      data = await YZData.at(tx.logs[0].args.addr);
      assert.ok(data);
      factory = await YZDataRequestFactory.deployed();
      tx = await factory.createYZDataRequest(data.address)
      drequest = await YZDataRequest.at(tx.logs[0].args.addr);
      assert.ok(drequest);

      pkey = '0xb99f1f5b552cd85cdb3b8b5800d0db8b5d38ca50921471100cb3e07ada908a9dae699470568fa76f1b69002f7367ba18d530f34d7d80426eb28c692921839703';
      await cert.register(accounts[0], pkey);
    })

    it('add program', async() =>{
      tx = await program_proxy.upload_program("test_name", "test_desc", "test_url", 0, "0x3e0d3a43f4f45ba7a1234759c2ffa4028a44599d4ab29bec532bd2057c0f9141")
      program_hash = tx.logs[0].args.hash
    })

    it('test request', async()=>{
      let secret = '0x9fbe7febcd5c9bd7e50a51ca03652271fd0455a800bf331e02a85e6223e8493905e8282f68e70b56eef6e51a3c69f453d8c5dcdb3ac36144e5e810a41202a478d4a6b4d57db8bb6ea0931c5907037eaac6ed9980426479ec401157bd649bc33e3eefbec1354fb8aa4feca5f1681aeed1581c201ab7f73c636051f63f4221183f51f2c02591a47322cf57055e18e63aa246f5c6d9ab2c28b233b8d807b843e9111cc110dfdc8ffcf7ad8afffd4848832a84';
      let input = '0x840e4c7ba276d0bf8531f12966521186fdee602c8de20d5f637b66fd118ae18dbfc52eadeb509541a1ed4058f94c516838a7b21bfad63c5fd9e0e4adc9101d47dced0dea7cec546712880426dc3c9a427939eb4b23ac2454b63e124d3296a5a76f8c220460ad4adbe5a232e2716bc79d4473c1e3f2303d0cba787f94430ef283fec1381c8690d37ff4c8953f7e23d6db7804cf24';
      let forward_sig = '0x0ce588a6d240a4c6da1b9c887c32576fd4eb43b170d42d4fea01e8cdfc50be634fdce867af14b76f8fbade6ca03a42b74ea855a1f12e80ff71e2dd5f79879f6d1c';
      let gas_price = 10000000000;
      tx = await drequest.request_data(secret, input, forward_sig, program_hash, gas_price);
      let request_hash = tx.logs[0].args.request_hash

      let data_hash = '0x0e23386a8e0086ee7d5c2dc7b0c67a87354cc7aa4db218fe63b3139d0d9a91b6';
      let cost = 0;
      let result = '0xf5e6b4fc58154099f83522000a96a72efbc3ced94203b6a54cebe6de7b074049104fa4bf7c0066e03a3c2095f20cec59522640a88ac820418dcef40a570544e8fc5ec2b9e240003fa524410b298043c23239e79be592e66541061545a0f95b62d6acd63bf08e7d85de6dddfbc176f25fa29b741eddd2dd65169ecffcba159d98cdef5584bc7ccc1d6c00116ef307e27afb0b75c92e5add4cf5d2defd9a6efb6d3aba77bdd55526866f92a8714230a23e8b8f97d77f4d4bfadf70a32f7439541e6c0fd43170187cb1648f7ff92372b79a12e6194d379357b0720460647dc0003023285871556522ac552701168155fd81c7c457ce6601b227ca9e9eb7bf436cd2fc52fb79869b48de3e9aa54f3004f3d17cb9f9aac505b4271e8af8a8b14977068abb1a45edd434557df3a494337f5e81adb191e91d893017d0974ff72f44abebf6bc02df9cb50c4890ee3bb02bd1c963c34d93f067b2551c4b9d76a28a323b5c3d18cdbea0cde9f5646aa4eb18b5a3ab67ae7b3b8590364ec83cba773188851f6f7f9a6e880c8d3e121841928cc79a59f3f601331d4b2ddfafa88f35d0f6592ee3743af7a2232e9f501825105cd224bda8072601438205288c184b14e5c3b4ea414a6d2f53d0f0ebd0b9546efe4891baebf831131c40cec9d682f737450c9eba763787bad5c78fdc6a63c48523d7cd7c1035b345fbd5a245c60a26eca6a6b30a98c0dff88cc57c7f1917b776874b29436fe50d18a237f3e8c082bf7af9b20fbfb6ce2be2b9166cd600ce869bf79fe1174f37b4fff255260f308c4434aad5c021f16895728de227df5ddbe84220751747da1976edf96d29876eeff9e4dcd0eb2095fe61fe72b1741a12b68184e91baba750aaad306a3e03a7be8140921bd7f652342d720932279320a1aa5d40219146587ffec38a02df53fe037ad8cf3d6215fd2561702d8c610336b13a8065a81a4957b91dbc481ca8f9e267bfb08b4b007df3eef0dfe52a7a11be5c7c8d4f512e3167756cb9b6bf7c5adc767ec69718d6c9c699d9b145ed6693c25af049ab8d6a5e6dbc2ed849ffd013d6b09849a84ec1bc2508211b959e83bc8cec0c46aaa560b2ce99477b1aa9e2622cdbe2cbc835f933fa88b67968c84bcff75d3869f55664acb657e5117e9d6fd6fb3da73a1a871e68f1872443012d9134b87895e0e21f84a50a894ef3aeeed2c4a0f54cdb198add4d7b67b5b6fb49ab03cc3fc912f34eedbc792669e3db253c779b39a1526c2606d43d0c55d6af0cdf29baed93b07fb6bf332a4356658ae7df1adeb90d1ea92eeb3be603ec03c7db134781e799ea36544240a0a7a75246f4916b0f160e013e8b84bdb434fa175d531201d6103c50512f8d3b0c66102cde79d8fe0f746ff90e6dfc025bf5f50f22e79a77bb641a336ccb3ae458084b92dc7b72c5bf01910c2f3a11bc3199da4577eb02a865da78123f35fb9bed1b02b0c15e44589aa0bf89867ba725b1d5233e76487d0b93fd7b7edb6a539794763310b20620b8024908051f60cf34c610be8067e02679a2c055ea480c081499e434158ac1c2cd4983cc0fdcd825204bca03cd2deacead9144e18f6d8454cdeca3be86b05ce933b8ffc56c3d72e3ef8200f25de04ce9f28c3b554018e8531508e8ff544684c6aad6560d0f42d17b3dc6d34f72ed020ad255e1027e4b6360c4d9749714f92380c6dd6d8d45306364f0feb6144e14ceefaf20a04f2c617b7f6e267c78895e9ddb5743f2e7ce160c99ae2ff6eacd4ac03b37faea1d3a65d092aa3ac9a237fa862a6b505c3f4ec7e7d1497c496b65d72bef1acfb68358bfc7bd6b273dc9cc93da6478ade8035171561738a18a02c5620c17ce2edc6ba573ddf4cb7de1cbc883c016024303268fb488aa03056478b061c3ae05919c7402f90e41fb9821bd8dc0f1b5c89abe3fa3a4827f1b39d8d46416f29e5636c5a836e74ce5f4c4736bb6bbe454e6efb6abfd2f8d3abc2ecf44290535498c750dcb771631041db5a36719c7ffd9879f484e1f31d87d304fe453af0c4054c3c791c626e997ee0d36a0c2901b18676fb8eb75c06aca451b98213f2172ae12906e269c99c1fe445b172e54439f5e0aae0529a54792895ac9fcf8c6a913455035eb3686286fc5d5597eccae41f886bc4501386c7690ed55d01a0e677bf1beaee393b9b859750c16c7585f5377d1f2b611a34f6c248e7c6cf38f5beeb0085df251a1ff0af3288bf5808f85d46f8dbacfcf5f9b0bca2d6ad8e05ba2d6c2170d0bdce65d8fe6a41abf1b2722425a3a0aeead1c26d853c3d31388ecfa2f678fce425a346b995b12eee5d540f6cd8b445c37c7da3da2da99bfffcc8b5a2971eb91c6fde8b7fdcd47ffd20e8cfd840c7267581fb014e1cbbe5dcdb6359ed9329c38393d606c2afeb8ea9fe44cafcabf1264e3de7b1650a3fc72459b26ca9a80de7a6784db17aea8d5763343d90bc4ebd3db2994a39a52c28ec959e68cce90b054a892a76b4d8a3baa454d721ead5de671e628af71ac081dcdcc8e7e00ade34bd58f74cd25fe3577ded45f71e9e8c7ead22e696be77c0eefb684219310c2dbe63ddfce8fe039cab48b4f7d446ad0e90b0d44119df2ae103720f78c3e968a2c9ec0ee1a813b965fdc55539c384d076c46f274090dc35cce57b7fe9c608b85a72349d11aacff171d3bd2e5f2cae511e1d9a21659c8897a9ea7e45204633417dada42c447550e72ea2d8ff1987df25396367efbce5f6d412b466c0447087ea88621b4727ce32374f279509845cc5577939aa9a2effd7e93617d7a27de1ba0e509bb8fb3e7f82435668139f085037fa35c2a4fb652e6a41451642248329c98ca566787b9d506beac0344a01e547cf6b02563f3553e06f8ba1bc16363e47daa5c52ed886007bbde726ad2df849dacbcc13d1e8987e816bbf4be2d0eeb381a65e128a3a55d06b315ef8d7130820401cbef988d712c17412241c25ef893148a5cce4b2e4f15dabb679c46063e9b76e03e463b116c3f4ca54bfd1e57b04f696eb5d5b21a98b81fde424a6becb8f23d59ee31ea548e21d5342acaa67d46d47d0851d4bb15d4db4d525e5dea741911bf7c2b9e26b8ab659a53f76598ac94ac3901c510898485132d088018473ff6b4f7b8e48a1a2c67238aec09b613c8b1b7d7593eeac9d06be24faee369724fdb6050b2eb8032d2ac869d3d6357c149c0379ec0a808010180799bb71cf768e443c6b46d7287d9b8c314defaea2d1a8674b7595f1db74273184e117307ddfee7106175abce4ad3b138c41e83243f4cca536978054fa9de9f40321e47f997d7372b09ab41f3baa7d7ec4b91d661ca6e15e5c28c7e421b776cd19fbc5801686e51b0cf9b2756d7c71edac97731078f3caf9384cfdd20cf0fcb04d1c2972427c194af4177a9d2ff42968aa4ca10571014ff706e3cfc36e039c212e6b32b6ea72908c5fd9a5caeb632bd8c72771df238b33a63956533b86d44ff3af3d161e59b73c8ef3baf772eed00a3e6e6c2c78d35037c4b8b8fa0736d18097a2387bd5eb5ee8863e20eb7e8191e60b1782e2a0cf98f29807db96e9686022dcaf181193eee71e3c77445c0d06ab0dd555deaf67c329add50e60804cf85c2ce1a6a81604a5147c06ceada1a0534647417aee2349971cef14dd767fcbce97876c34c30491b509fe59c4b3bfd2721399cb74ecab4b35c2729de104f1b494cd5eb3613d78eb6a18935f398ee4a4c45bfa5f897a8f385a1d1002ed236da61ce59234d52044598ac60e1f20fd3b7d33308db78c9fdc121b8ac37d738661a12dfbee6e2d2aecc3fe323a095de8ec0ca02361fc89867da4c96850dfaa3936b5bd6f1eb3a7b1ae770543bd2692b93c37765c9d01071aa96f3673e268f0efece95f23e85f388ffecb214debfdca420321369148262c5b13b8a068fb998b85cafbe9b9c845af49bfe43e2c56a573b78beb87e40b99a65be4625bba7939fb71430a5967168fb71806581633a29b91bd591c8ec3c26475efe7b41710f73195062bea164d4ea69e48b211c22ef5192c0aa28fbfbb110dcee9ab66227ac0c594191a69410f4f8bd44678a6ef5d5b5e38aaa1b0ce9041fbc640b3a0c32ad8ddb05b773';
      let signature = '0xb931369f43d0d0fb02d228f8ff495b9a989ec6b20357897f6724f13e1102b48608d715b2b6324010397fae81404115b447d1cb8732a39fbe18ea8af89a2cd1ca1b';

      let ret = await drequest.submit_result(request_hash, data_hash, cost, result, signature);
      await expectEvent(ret, "SubmitResult");
    })

	})

});
