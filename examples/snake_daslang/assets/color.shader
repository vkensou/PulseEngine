vert : "assets/color.vert.spv"
frag : "assets/color.frag.spv"
rasterizer : 
    cull_mode : "back"
    front_face : "clock_wise"
depth : 
    depth_test : true
    depth_write : true
    depth_op : "greater_equal"
    stencil_test : false
blend :
    alpha_to_coverage : false
    independent_blend : false
    attachments : [ 
        {
            enable : false
            src_factor : "one"
            dst_factor : "zero"
            src_alpha_factor : "one"
            dst_alpha_factor : "zero"
            blend_op : "add"
            blend_alpha_op : "add"
            color_mask : "rgba" 
        } ]
properties : [ 
    {
        name : "vpMatrix"
        type : "mat4"
        role : "non_material"
        set : 0
        binding : 0
        offset : 0
        size : 64
    }, 
    {
        name : "albedo"
        type : "float4"
        role : "material"
        set : 1
        binding : 0
        offset : 0
        size : 16
    }, 
    {
        name : "wMatrix"
        type : "mat4"
        role : "non_material"
        set : 2
        binding : 0
        offset : 0
        size : 64
    } ]
