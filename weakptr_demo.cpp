
namespace
{

    class OwnerClass
    {
        class PreMemberClass
        {
        public:
            explicit PreMemberClass(OwnerClass* owner)
                : owner_(owner)
            {
                // owner->weakptr_factory_����pre_member_class_instance_�������Ǳ���
                //owner_weakptr_ = owner->GetWeakPtr();
            }

            ~PreMemberClass()
            {
                // owner��Ȼ���ڣ��������Ա�����Ѿ������ˣ���ʱ�����owner_->OnMemberDestroy()�е����˱�������
                // ��Ա�������ܾͱ��ˣ������Ļ���ִ�н��Ҳ��һ������������
                owner_->OnMemberDestroy();

                // WeakPtr�ṩ��һ�ַ�ʽȥ�ж�owner�Ƿ�������״̬��
                if (owner_weakptr_)
                {
                    owner_weakptr_->OnMemberDestroy();
                }
            }

            void AttachOwnerWeakPtr(base::WeakPtr<OwnerClass> owner_weakptr)
            {
                owner_weakptr_ = owner_weakptr;
            }

        protected:
        private:
            OwnerClass* owner_ = nullptr;
            base::WeakPtr<OwnerClass> owner_weakptr_;
        };

        class TailMemberClass
        {
        public:
            TailMemberClass()
                : str_("PreMemberClass str")
                , int_(10000)
                , view_(new views::View)
                , raw_view_(new views::View)
            {
            }

            ~TailMemberClass()
            {
                view_.reset();
                delete raw_view_;
            }

            void InvokeTailMemberClassFunc()
            {
                LOG(INFO) << str_.c_str();
                LOG(INFO) << int_;
                LOG(INFO) << view_.get();
                raw_view_->GetVisibleBounds();
            }

        protected:
        private:
            int int_;
            std::string str_;
            scoped_ptr<views::View> view_;
            views::View* raw_view_ = nullptr;
        };

    public:
        OwnerClass()
            : pre_member_class_instance_(this)
            , protect_member_("this is a protected string")
            , weakptr_factory_(this)
        {
            pre_member_class_instance_.AttachOwnerWeakPtr(weakptr_factory_.GetWeakPtr());
        }

        ~OwnerClass()
        {
        }

        void OnMemberDestroy()
        {
            LOG(INFO) << protect_member_.c_str();

            tail_member_class_instance_.InvokeTailMemberClassFunc();
        }

        base::WeakPtr<OwnerClass> GetWeakPtr()
        {
            return weakptr_factory_.GetWeakPtr();
        }

    private:
        std::string protect_member_;
        PreMemberClass pre_member_class_instance_;
        TailMemberClass tail_member_class_instance_;
        base::WeakPtrFactory<OwnerClass> weakptr_factory_;
    };
}
