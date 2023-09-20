void	  init_seg(DATA_TYPE *seg, int seg_num);
void	  init_rhs(DATA_TYPE *rhs, DATA_TYPE *seg, int seg_num);
double    one_norm(DATA_TYPE *seg, int seg_num);
double    inf_norm(DATA_TYPE *seg, int seg_num);
double    init_eps(void);
void      mat_vec(DATA_TYPE *seg, int seg_num, DATA_TYPE *vec);
