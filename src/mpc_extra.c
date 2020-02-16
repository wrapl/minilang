mpc_parser_t *mpc_orv(int n, mpc_parser_t **parsers) {

  int i;
  va_list va;

  mpc_parser_t *p = mpc_undefined();

  p->type = MPC_TYPE_OR;
  p->data.or.n = n;
  p->data.or.xs = malloc(sizeof(mpc_parser_t*) * n);

  for (i = 0; i < n; i++) {
    p->data.or.xs[i] = *parsers++;
  }

  return p;
}

mpc_parser_t *mpc_andv(int n, mpc_fold_t f, mpc_parser_t **parsers) {

  int i;

  mpc_parser_t *p = mpc_undefined();

  p->type = MPC_TYPE_AND;
  p->data.and.n = n;
  p->data.and.f = f;
  p->data.and.xs = malloc(sizeof(mpc_parser_t*) * n);
  p->data.and.dxs = malloc(sizeof(mpc_dtor_t) * (n-1));

  for (i = 0; i < n; i++) {
    p->data.and.xs[i] = *parsers++;
  }
  for (i = 0; i < (n-1); i++) {
    p->data.and.dxs[i] = mpcf_dtor_null;
  }

  return p;
}
